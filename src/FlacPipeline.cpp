#include "FlacPipeline.h"
#include "FFmpegMemLoader.h"
#include "BatFFmpegRAII.h"
#include "Utils.h"
#include "FftTypes.h"
#include "AlignedAllocator.h"

#include <thread>
#include <mutex>
#include <map>
#include <atomic>
#include <cmath>
#include <vector>
#include <algorithm>
#include <type_traits>

extern "C" {
#include <libavutil/mathematics.h>
}

using namespace DecoderTypes;

namespace {
    struct DecodingChunk {
        int id;
        int64_t seek_pts;
        int64_t until_pts;
        double trim_start_sec;
        double trim_end_sec;
    };
    void apply_channel_matrix(SwrContext* swr, int input_channels, int target_channel_idx) {
        if (target_channel_idx >= 0 && target_channel_idx < input_channels) {
            std::vector<double> matrix(input_channels, 0.0);
            matrix[target_channel_idx] = 1.0;
            swr_set_matrix(swr, matrix.data(), 1);
        }
    }
    template <typename T>
    using AlignedVector = std::vector<T, AlignedAllocator<T, 64>>;
    template <typename T>
    void decode_worker_generic(
        AVFormatContext* formatCtx,
        int trackIndex,
        int channelIndex,
        DecodingChunk chunk,
        std::map<int, AlignedVector<T>>& all_results,
        std::mutex& results_mutex,
        std::atomic<bool>& has_error,
        QString& error_msg
    ) {
        if (has_error) return;
        AVStream* stream = formatCtx->streams[trackIndex];
        const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) return;
        AVCodecContextPtr codecCtx(avcodec_alloc_context3(codec));
        avcodec_parameters_to_context(codecCtx.get(), stream->codecpar);
        if (avcodec_open2(codecCtx.get(), codec, nullptr) < 0) return;
        AVChannelLayout out_layout;
        av_channel_layout_default(&out_layout, 1);
        AVSampleFormat targetFmt = std::is_same_v<T, double> ? AV_SAMPLE_FMT_DBL : AV_SAMPLE_FMT_FLT;
        SwrContext* swrCtxRaw = nullptr;
        swr_alloc_set_opts2(&swrCtxRaw, &out_layout, targetFmt, codecCtx->sample_rate,
            &codecCtx->ch_layout, codecCtx->sample_fmt, codecCtx->sample_rate, 0, nullptr);
        SwrContextPtr swrCtx(swrCtxRaw);
        apply_channel_matrix(swrCtx.get(), codecCtx->ch_layout.nb_channels, channelIndex);
        if (!swrCtx || swr_init(swrCtx.get()) < 0) return;
        if (av_seek_frame(formatCtx, trackIndex, chunk.seek_pts, AVSEEK_FLAG_BACKWARD) < 0) {}
        avcodec_flush_buffers(codecCtx.get());
        AlignedVector<T> threadPcm;
        try { threadPcm.reserve(65536); } catch (...) {}
        AVFramePtr frame(av_frame_alloc());
        AVPacketPtr pkt(av_packet_alloc());
        bool finished = false;
        double time_base = av_q2d(stream->time_base);
        double next_expected_time = static_cast<double>(chunk.seek_pts) * time_base;
        while (!finished && !has_error && av_read_frame(formatCtx, pkt.get()) >= 0) {
            if (pkt->stream_index == trackIndex) {
                if (pkt->pts != AV_NOPTS_VALUE && pkt->pts >= chunk.until_pts) {
                    finished = true;
                }
                if (avcodec_send_packet(codecCtx.get(), pkt.get()) >= 0) {
                    while (avcodec_receive_frame(codecCtx.get(), frame.get()) >= 0) {
                        double frame_start_time = 0.0;
                        if (frame->pts != AV_NOPTS_VALUE) {
                            frame_start_time = frame->pts * time_base;
                        } else {
                            frame_start_time = next_expected_time;
                        }
                        double frame_duration = (double)frame->nb_samples / codecCtx->sample_rate;
                        double frame_end_time = frame_start_time + frame_duration;
                        next_expected_time = frame_end_time;
                        if (frame_end_time <= chunk.trim_start_sec) {
                            av_frame_unref(frame.get());
                            continue;
                        }
                        if (frame_start_time >= chunk.trim_end_sec) {
                            av_frame_unref(frame.get());
                            finished = true;
                            break; 
                        }
                        int skip_samples = 0;
                        int valid_samples = frame->nb_samples;
                        if (frame_start_time < chunk.trim_start_sec) {
                            double diff = chunk.trim_start_sec - frame_start_time;
                            skip_samples = static_cast<int>(diff * codecCtx->sample_rate + 0.5);
                            if (skip_samples < 0) skip_samples = 0;
                            if (skip_samples >= frame->nb_samples) skip_samples = frame->nb_samples;
                        }
                        if (frame_end_time > chunk.trim_end_sec) {
                            double keep_duration = chunk.trim_end_sec - frame_start_time;
                            int keep_samples = static_cast<int>(keep_duration * codecCtx->sample_rate + 0.5);
                            if (keep_samples < 0) keep_samples = 0;
                            if (keep_samples < valid_samples) valid_samples = keep_samples;
                        }
                        int copy_count = valid_samples - skip_samples;
                        if (copy_count > 0) {
                            int max_out = swr_get_out_samples(swrCtx.get(), frame->nb_samples);
                            AlignedVector<T> buffer(max_out); 
                            uint8_t* out_data[1] = { reinterpret_cast<uint8_t*>(buffer.data()) };
                            int out_samples = swr_convert(swrCtx.get(), out_data, max_out, (const uint8_t**)frame->data, frame->nb_samples);
                            if (out_samples > 0) {
                                double ratio = (double)out_samples / frame->nb_samples;
                                int out_skip = static_cast<int>(skip_samples * ratio);
                                int out_copy = static_cast<int>(copy_count * ratio);
                                if (out_skip + out_copy > out_samples) out_copy = out_samples - out_skip;
                                if (out_copy > 0) {
                                    threadPcm.insert(threadPcm.end(), 
                                                     buffer.begin() + out_skip, 
                                                     buffer.begin() + out_skip + out_copy);
                                }
                            }
                        }
                        av_frame_unref(frame.get());
                    }
                }
            }
            av_packet_unref(pkt.get());
        }
        int delayed = swr_get_delay(swrCtx.get(), codecCtx->sample_rate);
        if (delayed > 0) {
            int max_out = swr_get_out_samples(swrCtx.get(), delayed);
            AlignedVector<T> buffer(max_out);
            uint8_t* out_data[1] = { reinterpret_cast<uint8_t*>(buffer.data()) };
            int out_samples = swr_convert(swrCtx.get(), out_data, max_out, nullptr, 0);
            if (out_samples > 0) {
                threadPcm.insert(threadPcm.end(), buffer.begin(), buffer.begin() + out_samples);
            }
        }
        {
            std::lock_guard<std::mutex> lock(results_mutex);
            all_results[chunk.id] = std::move(threadPcm);
        }
    }
    template <typename T>
    void decode_worker_memory_wrapper(
        int threadId,
        SharedFileBuffer fileData,
        int trackIndex,
        int channelIndex,
        DecodingChunk chunk,
        std::map<int, AlignedVector<T>>& all_results,
        std::mutex& results_mutex,
        std::atomic<bool>& has_error,
        QString& error_msg,
        LogCallback logCb
    ) {
        MemReaderState ioState;
        AVFormatContext* formatCtxRaw = FFmpegMemLoader::createFormatContextFromMemory(fileData, &ioState);
        if (!formatCtxRaw) { has_error = true; error_msg = FlacPipeline::tr("无法从内存打开流"); return; }
        std::unique_ptr<AVFormatContext, void(*)(AVFormatContext*)> formatCtx(
            formatCtxRaw, [](AVFormatContext* ptr){ FFmpegMemLoader::cleanup(ptr); }
        );
        if (avformat_find_stream_info(formatCtx.get(), nullptr) < 0) { has_error = true; error_msg = FlacPipeline::tr("无法读取流信息"); return; }
        decode_worker_generic<T>(formatCtx.get(), trackIndex, channelIndex, chunk, all_results, results_mutex, has_error, error_msg);
    }
    template <typename T>
    void decode_worker_file_wrapper(
        int threadId,
        QString filePath,
        int trackIndex,
        int channelIndex,
        DecodingChunk chunk,
        std::map<int, AlignedVector<T>>& all_results,
        std::mutex& results_mutex,
        std::atomic<bool>& has_error,
        QString& error_msg,
        LogCallback logCb
    ) {
        AVFormatContext* formatCtxRaw = nullptr;
        if (avformat_open_input(&formatCtxRaw, filePath.toUtf8().constData(), nullptr, nullptr) != 0) {
            has_error = true; error_msg = FlacPipeline::tr("无法打开文件"); return;
        }
        BatFFmpeg::BatFormatContextPtr formatCtx(formatCtxRaw);
        if (avformat_find_stream_info(formatCtx.get(), nullptr) < 0) { has_error = true; error_msg = FlacPipeline::tr("无法读取流信息"); return; }
        decode_worker_generic<T>(formatCtx.get(), trackIndex, channelIndex, chunk, all_results, results_mutex, has_error, error_msg);
    }
}

bool FlacPipeline::execute(
    const QString& filePath, 
    int trackIndex, 
    int channelIndex, 
    int sourceBitDepth,
    PcmDataVariant& outPcmData, 
    DecoderTypes::AudioMetadata& outMetadata, 
    DecoderTypes::LogCallback logCb, 
    ExecutionMode mode,
    double startSec,
    double endSec
) {
    AVFormatContext* formatCtxRaw = nullptr;
    if (avformat_open_input(&formatCtxRaw, filePath.toUtf8().constData(), nullptr, nullptr) != 0) return false;
    AVFormatContextPtr formatCtx(formatCtxRaw);
    if (avformat_find_stream_info(formatCtx.get(), nullptr) < 0) return false;
    AVStream* stream = formatCtx->streams[trackIndex];
    double time_base = av_q2d(stream->time_base);
    int64_t file_duration_ts = stream->duration;
    if (file_duration_ts == AV_NOPTS_VALUE && formatCtx->duration != AV_NOPTS_VALUE) {
        file_duration_ts = av_rescale_q(formatCtx->duration, AV_TIME_BASE_Q, stream->time_base);
    }
    double file_duration_sec = file_duration_ts * time_base;
    double effectiveEnd = (endSec > 0.001) ? endSec : file_duration_sec;
    if (effectiveEnd > file_duration_sec) effectiveEnd = file_duration_sec;
    double rangeDuration = effectiveEnd - startSec;
    if (rangeDuration < 0) rangeDuration = 0;
    outMetadata.preciseDurationMicroseconds = static_cast<long long>(rangeDuration * 1000000.0);
    const double THRESHOLD_SECONDS = 1.0; 
    if (rangeDuration < THRESHOLD_SECONDS) return false; 
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 2;
    if (logCb) logCb(QString(FlacPipeline::tr("        [线程] %1 线程")).arg(num_threads));
    std::vector<DecodingChunk> chunks;
    double chunk_sec = rangeDuration / num_threads;
    for(unsigned int i=0; i<num_threads; ++i) {
        double t_start = startSec + i * chunk_sec;
        double t_end = startSec + (i + 1) * chunk_sec;
        if (i == num_threads - 1) {
            t_end = effectiveEnd;
        }
        int64_t pts_seek = av_rescale_q(static_cast<int64_t>(t_start * AV_TIME_BASE), AV_TIME_BASE_Q, stream->time_base);
        int64_t pts_until = av_rescale_q(static_cast<int64_t>(t_end * AV_TIME_BASE), AV_TIME_BASE_Q, stream->time_base);
        if (i == num_threads - 1 && endSec <= 0.001) {
            pts_until = std::numeric_limits<int64_t>::max(); 
        }
        chunks.push_back({(int)i, pts_seek, pts_until, t_start, t_end});
    }
    std::mutex mutex;
    std::vector<std::thread> threads;
    std::atomic<bool> has_error(false);
    QString error_msg;
    SharedFileBuffer fileBuffer;
    if (mode == ExecutionMode::LoadToRam) {
        if (logCb) logCb(getCurrentTimestamp() + FlacPipeline::tr(" 开始加载文件"));
        fileBuffer = FFmpegMemLoader::loadFileToMemory(filePath);
        if (!fileBuffer) {
            if (logCb) logCb(FlacPipeline::tr("        文件加载失败 回退到单线程模式"));
            return false;
        }
    }
    if (logCb) logCb(getCurrentTimestamp() + FlacPipeline::tr(" 开始并行解码"));
    bool useDouble = (sourceBitDepth > 32);
    auto process_template = [&](auto dummy_type) -> bool {
        using T = decltype(dummy_type);
        std::map<int, AlignedVector<T>> results;
        for(unsigned int i=0; i<num_threads; ++i) {
            if (mode == ExecutionMode::LoadToRam) {
                threads.emplace_back(decode_worker_memory_wrapper<T>, i, fileBuffer, trackIndex, channelIndex, chunks[i], std::ref(results), std::ref(mutex), std::ref(has_error), std::ref(error_msg), logCb);
            } else {
                threads.emplace_back(decode_worker_file_wrapper<T>, i, filePath, trackIndex, channelIndex, chunks[i], std::ref(results), std::ref(mutex), std::ref(has_error), std::ref(error_msg), logCb);
            }
        }
        for(auto& t : threads) t.join();
        if (has_error) {
            if (logCb) logCb(FlacPipeline::tr("        并行解码错误 ") + error_msg + FlacPipeline::tr(" 回退到单线程模式"));
            return false;
        }
        AlignedVector<T> finalPcm;
        size_t total_size = 0;
        for(const auto& pair : results) total_size += pair.second.size();
        finalPcm.reserve(total_size);
        for(const auto& pair : results) {
            finalPcm.insert(finalPcm.end(), pair.second.begin(), pair.second.end());
        }
        size_t currentSize = finalPcm.size();
        size_t bytes = currentSize * sizeof(T);
        size_t alignedBytes = (bytes + 63) & ~63;
        if (alignedBytes > bytes) {
            size_t alignedCount = alignedBytes / sizeof(T);
            finalPcm.resize(alignedCount, 0);
        }
        outPcmData = std::move(finalPcm);
        return true;
    };
    if (useDouble) return process_template(double{});
    else return process_template(float{});
}