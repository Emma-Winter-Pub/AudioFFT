#include "ApePipeline.h"
#include "BatFFmpegRAII.h"
#include "FFmpegMemLoader.h"
#include "AlignedAllocator.h"
#include "Utils.h"

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <map>
#include <limits>

extern "C" {
#include <libavutil/opt.h>
}

using namespace DecoderTypes;

namespace {
    constexpr int ANCHOR_SIZE = 8192;
    constexpr size_t MAX_SEARCH_LIMIT = 1024 * 1024;
    constexpr double PREROLL_SEC = 1.0;
    constexpr double POSTROLL_SEC = 1.0;
    struct ChunkDef {
        int id;
        double logic_start;
        double logic_end;
        double phys_start;
        double phys_end;
    };
    template <typename T>
    struct ChunkResult {
        int id;
        std::vector<T, AlignedAllocator<T, 64>> raw_data;
        size_t valid_offset = 0;
        size_t valid_length = 0;
    };
    void apply_channel_matrix(SwrContext* swr, int input_channels, int target_channel_idx) {
        if (target_channel_idx >= 0 && target_channel_idx < input_channels) {
            std::vector<double> matrix(input_channels, 0.0);
            matrix[target_channel_idx] = 1.0;
            swr_set_matrix(swr, matrix.data(), 1);
        }
    }
    template <typename T>
    bool is_silence_anchor(const std::vector<T, AlignedAllocator<T, 64>>& data, size_t offset, size_t len) {
        if (offset + len > data.size()) return true;
        double threshold = static_cast<double>(std::numeric_limits<T>::epsilon()) * 4.0;
        for (size_t i = 0; i < len; ++i) {
            if (std::abs(static_cast<double>(data[offset + i])) > threshold) return false;
        }
        return true;
    }
    template <typename T>
    int64_t find_pattern(const std::vector<T, AlignedAllocator<T, 64>>& haystack, 
                         const std::vector<T, AlignedAllocator<T, 64>>& source_for_needle,
                         size_t needle_offset, size_t needle_len) 
    {
        if (needle_offset + needle_len > source_for_needle.size()) return -1;
        if (haystack.size() < needle_len) return -1;
        auto needle_begin = source_for_needle.begin() + needle_offset;
        auto needle_end = needle_begin + needle_len;
        size_t limit = std::min(haystack.size() - needle_len, MAX_SEARCH_LIMIT);
        auto search_limit_it = haystack.begin() + limit;
        auto it1 = std::search(haystack.begin(), search_limit_it, 
                              needle_begin, needle_end);
        if (it1 == search_limit_it) {
            return -1;
        }
        if (it1 + 1 < search_limit_it) {
            auto it2 = std::search(it1 + 1, search_limit_it,
                                   needle_begin, needle_end);
            if (it2 != search_limit_it) {
                return -2; 
            }
        }
        return std::distance(haystack.begin(), it1);
    }
    std::vector<double> calculate_cuts(double duration, int n) {
        std::vector<double> cuts;
        cuts.reserve(n + 1);
        cuts.push_back(0.0);
        if (n <= 1) { 
            cuts.push_back(duration); 
            return cuts; 
        }
        int64_t total_sec = static_cast<int64_t>(duration);
        int64_t base = total_sec / n;
        int64_t remainder = total_sec % n;
        int64_t current_pos = 0;
        for (int i = 0; i < remainder; ++i) {
            current_pos += base + 1;
            cuts.push_back(static_cast<double>(current_pos));
        }
        for (int i = 0; i < n - remainder; ++i) {
            current_pos += base;
            cuts.push_back(static_cast<double>(current_pos));
        }
        cuts.back() = duration;  
        return cuts;
    }
    template <typename T>
    void decode_segment_ffmpeg(
        AVFormatContext* fmt_ctx,
        int track_idx,
        int ch_idx,
        double start_sec,
        double end_sec,
        std::vector<T, AlignedAllocator<T, 64>>& out_pcm,
        std::atomic<bool>& has_error
    ) {
        if (has_error) return;
        AVStream* stream = fmt_ctx->streams[track_idx];
        const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) { has_error = true; return; }
        BatFFmpeg::BatCodecContextPtr codecCtx = BatFFmpeg::make_codec_context(codec);
        if (avcodec_parameters_to_context(codecCtx.get(), stream->codecpar) < 0) { has_error = true; return; }
        if (avcodec_open2(codecCtx.get(), codec, nullptr) < 0) { has_error = true; return; }
        AVChannelLayout out_layout = AV_CHANNEL_LAYOUT_MONO;
        AVSampleFormat targetFmt = std::is_same_v<T, double> ? AV_SAMPLE_FMT_DBL : AV_SAMPLE_FMT_FLT;
        SwrContext* swr_raw = nullptr;
        swr_alloc_set_opts2(&swr_raw, &out_layout, targetFmt, codecCtx->sample_rate,
                            &codecCtx->ch_layout, codecCtx->sample_fmt, codecCtx->sample_rate, 0, nullptr);
        BatFFmpeg::BatSwrContextPtr swrCtx(swr_raw);
        apply_channel_matrix(swrCtx.get(), codecCtx->ch_layout.nb_channels, ch_idx);
        if (!swrCtx || swr_init(swrCtx.get()) < 0) { has_error = true; return; }
        int64_t seek_pts = static_cast<int64_t>(start_sec / av_q2d(stream->time_base));
        if (av_seek_frame(fmt_ctx, track_idx, seek_pts, AVSEEK_FLAG_BACKWARD) < 0) {
            av_seek_frame(fmt_ctx, track_idx, 0, AVSEEK_FLAG_BACKWARD);
        }
        avcodec_flush_buffers(codecCtx.get());
        BatFFmpeg::BatPacketPtr pkt = BatFFmpeg::make_packet();
        BatFFmpeg::BatFramePtr frame = BatFFmpeg::make_frame();
        double time_base = av_q2d(stream->time_base);
        bool finished = false;
        double next_expected_time = start_sec; 
        out_pcm.clear();
        try { out_pcm.reserve(static_cast<size_t>((end_sec - start_sec) * codecCtx->sample_rate * 1.2)); } catch(...) {}
        while (!finished && !has_error && av_read_frame(fmt_ctx, pkt.get()) >= 0) {
            if (pkt->stream_index == track_idx) {
                if (pkt->pts != AV_NOPTS_VALUE && (pkt->pts * time_base) > end_sec + 2.0) {
                    finished = true;
                }
                if (avcodec_send_packet(codecCtx.get(), pkt.get()) >= 0) {
                    while (avcodec_receive_frame(codecCtx.get(), frame.get()) >= 0) {
                        double frame_start = 0.0;
                        if (frame->pts != AV_NOPTS_VALUE) {
                            frame_start = frame->pts * time_base;
                        } else {
                            frame_start = next_expected_time;
                        }
                        double frame_len_sec = (double)frame->nb_samples / codecCtx->sample_rate;
                        double frame_end = frame_start + frame_len_sec;
                        next_expected_time = frame_end;
                        if (frame_end <= start_sec) {
                            av_frame_unref(frame.get());
                            continue;
                        }
                        int skip_samples = 0;
                        if (frame_start < start_sec) {
                            double diff = start_sec - frame_start;
                            skip_samples = static_cast<int>(diff * codecCtx->sample_rate);
                            if (skip_samples < 0) skip_samples = 0;
                            if (skip_samples >= frame->nb_samples) skip_samples = frame->nb_samples;
                        }
                        int valid_samples = frame->nb_samples - skip_samples;
                        if (valid_samples <= 0) {
                            av_frame_unref(frame.get());
                            continue;
                        }
                        int max_out = swr_get_out_samples(swrCtx.get(), frame->nb_samples);
                        std::vector<T> temp_buffer(max_out);
                        uint8_t* out_data[1] = { reinterpret_cast<uint8_t*>(temp_buffer.data()) };
                        int out_samples = swr_convert(swrCtx.get(), out_data, max_out, (const uint8_t**)frame->data, frame->nb_samples);
                        if (out_samples > 0) {
                            double ratio = (double)out_samples / frame->nb_samples;
                            int out_skip = static_cast<int>(skip_samples * ratio);
                            int out_keep = out_samples - out_skip;
                            if (out_keep > 0) {
                                out_pcm.insert(out_pcm.end(), 
                                               temp_buffer.begin() + out_skip, 
                                               temp_buffer.begin() + out_skip + out_keep);
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
            size_t old_size = out_pcm.size();
            out_pcm.resize(old_size + delayed);
            uint8_t* out_data[1] = { reinterpret_cast<uint8_t*>(out_pcm.data() + old_size) };
            int ret = swr_convert(swrCtx.get(), out_data, delayed, nullptr, 0);
            if (ret < delayed) out_pcm.resize(old_size + ret);
        }
    }
    template <typename T>
    void worker_entry(
        int id, 
        SharedFileBuffer memFile, 
        QString filePath, 
        bool isMemMode,
        int trackIdx, int chIdx, 
        double start, double end,
        std::vector<ChunkResult<T>>* results,
        std::atomic<bool>* has_error
    ) {
        AVFormatContext* fmt = nullptr;
        MemReaderState ioState = {nullptr, 0, 0};
        if (isMemMode) {
            fmt = FFmpegMemLoader::createFormatContextFromMemory(memFile, &ioState);
        } else {
            avformat_open_input(&fmt, filePath.toUtf8().constData(), nullptr, nullptr);
        }
        if (!fmt) { *has_error = true; return; }
        BatFFmpeg::BatFormatContextPtr ctxPtr(fmt); 
        if (avformat_find_stream_info(fmt, nullptr) < 0) { *has_error = true; return; }
        decode_segment_ffmpeg<T>(fmt, trackIdx, chIdx, start, end, (*results)[id].raw_data, *has_error);
        if (!*has_error) {
            (*results)[id].valid_length = (*results)[id].raw_data.size();
            (*results)[id].valid_offset = 0;
        }
    }
    int get_w_limit(int t) {
        if (t >= 5 && t <= 8) return 1;
        if (t >= 9 && t <= 13) return 2;
        if (t >= 14 && t <= 19) return 3;
        if (t >= 20 && t <= 25) return 4;
        if (t >= 26 && t <= 38) return 5;
        if (t >= 39 && t <= 44) return 6;
        if (t >= 45 && t <= 50) return 7;
        if (t >= 51 && t <= 56) return 8;
        if (t >= 57 && t <= 62) return 9;
        return 10;
    }
    template <typename T>
    class ApeNewRunner {
    public:
        static bool run(
            const QString& filePath,
            SharedFileBuffer fileBuffer,
            int trackIndex,
            int channelIndex,
            ExecutionMode mode,
            double duration,
            int sampleRate,
            PcmDataVariant& outData,
            LogCallback logCb
        ) {
            double total_sec = duration;
            if (total_sec < 120.0) {
                if (logCb) logCb(ApePipeline::tr("        [检查] 音频时长太短 使用单线程"));
                return false;
            }
            int theoretical_threads = static_cast<int>(total_sec / 3.0);
            int os_threads = std::thread::hardware_concurrency();
            if (os_threads == 0) os_threads = 2;
            if (os_threads <= 1) {
                if (logCb) logCb(ApePipeline::tr("        [检查] 系统为单线程 使用单线程"));
                return false;
            }
            int t = std::min(theoretical_threads, os_threads);
            if (t < 2) return false;
            int n = t;
            int w = 0;
            int x = 0;
            int y = 0;
            std::map<int, double> shift_offsets;
            while (true) {
                if (logCb) logCb(ApePipeline::tr("        [执行] 使用%1线程 重试%2次 短音频偏移%3次 长音频偏移%4次").arg(n).arg(w).arg(x).arg(y));
                std::vector<double> cuts = calculate_cuts(total_sec, n);
                if (!shift_offsets.empty()) {
                    for (auto const& [idx, offset] : shift_offsets) {
                        if (idx >= 0 && idx < cuts.size()) {
                            cuts[idx] += offset;
                            if (cuts[idx] < 0) cuts[idx] = 0;
                            if (cuts[idx] > total_sec) cuts[idx] = total_sec;
                        }
                    }
                }
                std::vector<ChunkDef> chunk_defs(n);
                for (int i = 0; i < n; ++i) {
                    chunk_defs[i].id = i;
                    double l_start = cuts[i];
                    double l_end = cuts[i+1];
                    chunk_defs[i].logic_start = l_start;
                    chunk_defs[i].logic_end = l_end;
                    chunk_defs[i].phys_start = std::max(0.0, l_start - (i == 0 ? 0.0 : PREROLL_SEC));
                    chunk_defs[i].phys_end = std::min(total_sec, l_end + (i == n - 1 ? 0.0 : POSTROLL_SEC));
                }
                std::vector<ChunkResult<T>> results(n);
                for(int i=0; i<n; ++i) results[i].id = i;
                std::atomic<bool> has_error(false);
                std::vector<std::thread> workers;
                for (int i = 0; i < n; ++i) {
                    workers.emplace_back(
                        worker_entry<T>,
                        i, fileBuffer, filePath, (mode == ExecutionMode::LoadToRam),
                        trackIndex, channelIndex,
                        chunk_defs[i].phys_start, chunk_defs[i].phys_end,
                        &results, &has_error
                    );
                }
                for (auto& worker : workers) worker.join();
                if (has_error) {
                }
                bool j1_all_valid = true;
                int problem_cut_index = -1;
                for (int i = 0; i < n - 1; ++i) {
                    auto& curr = results[i];
                    double time_offset = chunk_defs[i].logic_end - chunk_defs[i].phys_start;
                    size_t anchor_start_idx = static_cast<size_t>(time_offset * sampleRate);
                    if (anchor_start_idx + ANCHOR_SIZE > curr.raw_data.size()) {
                        j1_all_valid = false;
                        problem_cut_index = i + 1;
                        break;
                    }
                    if (is_silence_anchor(curr.raw_data, anchor_start_idx, ANCHOR_SIZE)) {
                        j1_all_valid = false;
                        problem_cut_index = i + 1;
                        break;
                    }
                }
                if (!j1_all_valid) {
                    goto LABEL_U;
                }
                {
                    bool k1_all_success = true;
                    std::vector<int64_t> match_positions(n - 1);
                    for (int i = 0; i < n - 1; ++i) {
                        auto& curr = results[i];
                        auto& next = results[i+1];
                        double time_offset = chunk_defs[i].logic_end - chunk_defs[i].phys_start;
                        size_t anchor_start_idx = static_cast<size_t>(time_offset * sampleRate);
                        if (anchor_start_idx + ANCHOR_SIZE > curr.raw_data.size()) {
                            k1_all_success = false;
                            problem_cut_index = i + 1;
                            break;
                        }
                        std::vector<T, AlignedAllocator<T, 64>> anchor(
                            curr.raw_data.begin() + anchor_start_idx, 
                            curr.raw_data.begin() + anchor_start_idx + ANCHOR_SIZE
                        );
                        int64_t pos = find_pattern(next.raw_data, anchor, 0, ANCHOR_SIZE);
                        if (pos < 0) {
                            k1_all_success = false;
                            problem_cut_index = i + 1;
                            if (pos == -2 && logCb) {
                                logCb(ApePipeline::tr("        [警告] 存在多处匹配 放弃并行解码"));
                            }
                            break;
                        }
                        match_positions[i] = pos;
                    }
                    if (!k1_all_success) {
                        goto LABEL_U;
                    }
                    for (int i = 0; i < n - 1; ++i) {
                        double time_offset = chunk_defs[i].logic_end - chunk_defs[i].phys_start;
                        size_t cut_point = static_cast<size_t>(time_offset * sampleRate);
                        if (cut_point > results[i].raw_data.size()) cut_point = results[i].raw_data.size();
                        if (cut_point > results[i].valid_offset) {
                            results[i].valid_length = cut_point - results[i].valid_offset;
                        } else {
                            results[i].valid_length = 0;
                        }
                        int64_t match = match_positions[i];
                        results[i+1].valid_offset = match;
                        if (results[i+1].raw_data.size() > match) {
                            results[i+1].valid_length = results[i+1].raw_data.size() - match;
                        } else {
                            results[i+1].valid_length = 0;
                        }
                    }
                    size_t total_len = 0;
                    for (const auto& r : results) total_len += r.valid_length;
                    std::vector<T, AlignedAllocator<T, 64>> final_pcm;
                    try { final_pcm.resize(total_len); } catch(...) { return false; }
                    T* dst = final_pcm.data();
                    size_t write_pos = 0;
                    for (const auto& r : results) {
                        if (r.valid_length > 0) {
                            std::memcpy(dst + write_pos, 
                                        r.raw_data.data() + r.valid_offset, 
                                        r.valid_length * sizeof(T));
                            write_pos += r.valid_length;
                        }
                        const_cast<std::vector<T, AlignedAllocator<T, 64>>&>(r.raw_data).clear();
                    }
                    outData = std::move(final_pcm);
                    return true;
                }
            LABEL_U:
                for (auto& r : results) {
                    std::vector<T, AlignedAllocator<T, 64>>().swap(r.raw_data);
                }
                if (logCb) logCb(ApePipeline::tr("        [警告] 验证失败 进入错误处理流程"));
                if (t >= 2 && t <= 4) goto LABEL_V;
                goto LABEL_W;
            LABEL_W:
                w++; 
                {
                    int max_w = get_w_limit(t);
                    if (w <= max_w) {
                        n = n - 1;
                        shift_offsets.clear();
                        continue;
                    } else {
                        return false;
                    }
                }
            LABEL_V:
                if (total_sec >= 120.0 && total_sec <= 180.0) return false;
                if (total_sec > 180.0 && total_sec <= 600.0) goto LABEL_X;
                goto LABEL_Y;
            LABEL_X:
                x++;
                if (x == 1) {
                    if (problem_cut_index != -1) {
                        shift_offsets[problem_cut_index] -= 10.0; 
                        if (logCb) logCb(ApePipeline::tr("        [偏移] 分割点%1 向前移动10秒").arg(problem_cut_index));
                    }
                    continue; 
                } else {
                    return false;
                }
            LABEL_Y:
                y++;
                if (y == 1) {
                    if (problem_cut_index != -1) {
                        shift_offsets[problem_cut_index] = -10.0; 
                        if (logCb) logCb(ApePipeline::tr("        [偏移] 分割点%1 向前移动10秒").arg(problem_cut_index));
                    }
                    continue;
                } else if (y == 2) {
                    if (problem_cut_index != -1) {
                        shift_offsets[problem_cut_index] = 10.0; 
                        if (logCb) logCb(ApePipeline::tr("        [偏移] 分割点%1 向后移动10秒").arg(problem_cut_index));
                    }
                    continue;
                } else {
                    return false;
                }
            }
            return false;
        }
    };
}

bool ApePipeline::execute(
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
    Q_UNUSED(startSec); Q_UNUSED(endSec);
    AVFormatContext* fmtRaw = nullptr;
    if (avformat_open_input(&fmtRaw, filePath.toUtf8().constData(), nullptr, nullptr) != 0) return false;
    BatFFmpeg::BatFormatContextPtr fmtCtx(fmtRaw);
    if (avformat_find_stream_info(fmtCtx.get(), nullptr) < 0) return false;
    AVStream* stream = fmtCtx->streams[trackIndex];
    double duration = stream->duration * av_q2d(stream->time_base);
    int sampleRate = stream->codecpar->sample_rate;
    outMetadata.preciseDurationMicroseconds = static_cast<long long>(duration * 1000000.0);
    SharedFileBuffer fileBuffer;
    if (mode == ExecutionMode::LoadToRam) {
        if(logCb) logCb(getCurrentTimestamp() + tr(" 开始加载文件"));
        fileBuffer = FFmpegMemLoader::loadFileToMemory(filePath);
        if (!fileBuffer) return false;
    }
    bool useDouble = (sourceBitDepth > 32);
    try {
        if (useDouble) {
            return ApeNewRunner<double>::run(filePath, fileBuffer, trackIndex, channelIndex, mode, duration, sampleRate, outPcmData, logCb);
        } else {
            return ApeNewRunner<float>::run(filePath, fileBuffer, trackIndex, channelIndex, mode, duration, sampleRate, outPcmData, logCb);
        }
    } catch (...) {
        return false;
    }
}