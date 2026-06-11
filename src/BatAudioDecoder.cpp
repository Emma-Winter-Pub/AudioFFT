#include "BatAudioDecoder.h"
#include "BatFFmpegRAII.h"
#include "FFmpegMemLoader.h"

#include <libavutil/opt.h>
#include <algorithm>
#include <variant>

namespace {
template <typename T>
int decode_and_resample_packet_T(
    AVCodecContext* dec_ctx, 
    SwrContext* swr_ctx, 
    AVPacket* pkt, 
    BatFFmpeg::BatFramePtr& frame, 
    std::vector<T, AlignedAllocator<T, 64>>& pcm_data
    )
{
    int ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) return ret;
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame.get());
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return 0;
        if (ret < 0) return ret;
        const int out_nb_samples = frame->nb_samples;
        std::vector<T> buffer(out_nb_samples);
        uint8_t* out_data[1] = { reinterpret_cast<uint8_t*>(buffer.data()) };
        int converted_samples = swr_convert(swr_ctx, out_data, out_nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
        if (converted_samples > 0) {
            pcm_data.insert(pcm_data.end(), buffer.begin(), buffer.begin() + converted_samples);
        }
        av_frame_unref(frame.get());
    }
    return 0;
}
}

BatAudioDecoder::BatAudioDecoder() {}

BatAudioDecoder::~BatAudioDecoder() {}

std::optional<BatDecodedAudio> BatAudioDecoder::decode(const QString& filePath){
    AVFormatContext* format_ctx_raw = nullptr;
    if (avformat_open_input(&format_ctx_raw, filePath.toUtf8().constData(), nullptr, nullptr) != 0) {
        return std::nullopt;
    }
    BatFFmpeg::BatFormatContextPtr formatContext(format_ctx_raw);
    return decodeInternal(formatContext.get());
}

std::optional<BatDecodedAudio> BatAudioDecoder::decode(SharedFileBuffer buffer){
    if (!buffer || buffer->empty()) return std::nullopt;
    MemReaderState ioState;
    AVFormatContext* format_ctx_raw = FFmpegMemLoader::createFormatContextFromMemory(buffer, &ioState);
    if (!format_ctx_raw) return std::nullopt;
    std::unique_ptr<AVFormatContext, void(*)(AVFormatContext*)> formatContext(
        format_ctx_raw, 
        [](AVFormatContext* ptr){ FFmpegMemLoader::cleanup(ptr); }
    );
    return decodeInternal(formatContext.get());
}

std::optional<BatDecodedAudio> BatAudioDecoder::decodeInternal(AVFormatContext* formatContext){
    if (!formatContext) return std::nullopt;
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        return std::nullopt;
    }
    int audio_stream_index = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_index < 0) {
        return std::nullopt;
    }
    AVStream* audio_stream = formatContext->streams[audio_stream_index];
    AVRational time_base = audio_stream->time_base;
    int64_t last_pts_plus_duration = 0;
    const AVCodec* codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
    if (!codec) {
        return std::nullopt;
    }
    auto codec_ctx = BatFFmpeg::make_codec_context(codec);
    if (!codec_ctx || avcodec_parameters_to_context(codec_ctx.get(), audio_stream->codecpar) < 0) {
        return std::nullopt;
    }
    if (avcodec_open2(codec_ctx.get(), codec, nullptr) < 0) {
        return std::nullopt;
    }
    int bits = audio_stream->codecpar->bits_per_raw_sample;
    if (bits <= 0) bits = av_get_bytes_per_sample(static_cast<AVSampleFormat>(audio_stream->codecpar->format)) * 8;
    if (bits <= 0) bits = 16;
    bool useDouble = (bits > 32) || 
                     (audio_stream->codecpar->format == AV_SAMPLE_FMT_DBL) || 
                     (audio_stream->codecpar->format == AV_SAMPLE_FMT_DBLP);
    AVSampleFormat targetSwrFormat = useDouble ? AV_SAMPLE_FMT_DBL : AV_SAMPLE_FMT_FLT;
    SwrContext* swr_ctx_raw = nullptr;
    AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_MONO;
    swr_alloc_set_opts2(&swr_ctx_raw,
                        &out_ch_layout, targetSwrFormat, codec_ctx->sample_rate,
                        &codec_ctx->ch_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate,
                        0, nullptr);
    BatFFmpeg::BatSwrContextPtr swr_ctx(swr_ctx_raw);
    if (!swr_ctx || swr_init(swr_ctx.get()) < 0) {
        return std::nullopt;
    }
    auto packet = BatFFmpeg::make_packet();
    auto frame = BatFFmpeg::make_frame();
    size_t estimatedSamples = 0;
    double durationSecApprox = 0.0;
    if (formatContext->duration != AV_NOPTS_VALUE) {
        durationSecApprox = formatContext->duration / (double)AV_TIME_BASE;
        estimatedSamples = static_cast<size_t>(durationSecApprox * codec_ctx->sample_rate * 1.1);
    }
    PcmData32 pcm32;
    PcmData64 pcm64;
    try {
        if (useDouble) {
            if (estimatedSamples > 0) {
                size_t alignedSize = (estimatedSamples + 7) & ~7;
                pcm64.reserve(alignedSize);
            }
        } else {
            if (estimatedSamples > 0) {
                size_t alignedSize = (estimatedSamples + 15) & ~15;
                pcm32.reserve(alignedSize);
            }
        }
    } catch (...) {
    }
    while (av_read_frame(formatContext, packet.get()) >= 0) {
        if (packet->stream_index == audio_stream_index) {
            if (packet->pts != AV_NOPTS_VALUE) {
                int64_t current_end_ts = packet->pts + packet->duration;
                last_pts_plus_duration = std::max(last_pts_plus_duration, current_end_ts);
            }
            if (useDouble) {
                decode_and_resample_packet_T<double>(codec_ctx.get(), swr_ctx.get(), packet.get(), frame, pcm64);
            } else {
                decode_and_resample_packet_T<float>(codec_ctx.get(), swr_ctx.get(), packet.get(), frame, pcm32);
            }
        }
        av_packet_unref(packet.get());
    }
    if (useDouble) {
        decode_and_resample_packet_T<double>(codec_ctx.get(), swr_ctx.get(), nullptr, frame, pcm64);
    } else {
        decode_and_resample_packet_T<float>(codec_ctx.get(), swr_ctx.get(), nullptr, frame, pcm32);
    }
    int delayed = swr_get_delay(swr_ctx.get(), codec_ctx->sample_rate);
    if (delayed > 0) {
        int max_out = swr_get_out_samples(swr_ctx.get(), delayed);
        if (useDouble) {
            std::vector<double> buffer(max_out);
            uint8_t* out_data[1] = { reinterpret_cast<uint8_t*>(buffer.data()) };
            int out_samples = swr_convert(swr_ctx.get(), out_data, max_out, nullptr, 0);
            if (out_samples > 0) {
                pcm64.insert(pcm64.end(), buffer.begin(), buffer.begin() + out_samples);
            }
        } else {
            std::vector<float> buffer(max_out);
            uint8_t* out_data[1] = { reinterpret_cast<uint8_t*>(buffer.data()) };
            int out_samples = swr_convert(swr_ctx.get(), out_data, max_out, nullptr, 0);
            if (out_samples > 0) {
                pcm32.insert(pcm32.end(), buffer.begin(), buffer.begin() + out_samples);
            }
        }
    }
    BatDecodedAudio result;
    result.sourceBitDepth = bits;
    result.sampleRate = codec_ctx->sample_rate;
    double ptsDuration = 0.0;
    if (last_pts_plus_duration > 0) {
        ptsDuration = static_cast<double>(last_pts_plus_duration) * av_q2d(time_base);
    } else {
        ptsDuration = (formatContext->duration == AV_NOPTS_VALUE) ? 0 : (formatContext->duration / (double)AV_TIME_BASE);
    }
    double pcmDuration = 0.0;
    size_t totalSamples = useDouble ? pcm64.size() : pcm32.size();
    if (result.sampleRate > 0 && totalSamples > 0) {
        pcmDuration = static_cast<double>(totalSamples) / result.sampleRate;
    }
    if (pcmDuration > 0) {
        if (ptsDuration > 0.001) {
            if (pcmDuration > ptsDuration + 0.002) {
                result.durationSec = ptsDuration;
            } else {
                result.durationSec = pcmDuration;
            }
        } else {
            result.durationSec = pcmDuration;
        }
    } else {
        result.durationSec = ptsDuration;
    }
    bool isEmpty = true;
    if (useDouble) {
        if (!pcm64.empty()) {
            result.pcmData = std::move(pcm64);
            isEmpty = false;
        }
    } else {
        if (!pcm32.empty()) {
            result.pcmData = std::move(pcm32);
            isEmpty = false;
        }
    }
    if (isEmpty) {
        return std::nullopt;
    }
    return result;
}