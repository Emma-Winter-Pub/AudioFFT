#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include <memory>

namespace BatchFullLoadFFmpegRAII {
    struct FormatContextDeleter {void operator()(AVFormatContext* ctx) const noexcept {if (ctx) avformat_close_input(&ctx);}};
    struct CodecContextDeleter {void operator()(AVCodecContext* ctx) const noexcept {if (ctx) avcodec_free_context(&ctx);}};
    struct FrameDeleter {void operator()(AVFrame* frame) const noexcept {if (frame) av_frame_free(&frame);}};
    struct PacketDeleter {void operator()(AVPacket* pkt) const noexcept {if (pkt) av_packet_free(&pkt);}};
    struct SwrContextDeleter {void operator()(SwrContext* swr) const noexcept {if (swr) swr_free(&swr);}};
    using BatchFullLoadFormatContextPtr = std::unique_ptr<AVFormatContext, FormatContextDeleter>;
    using BatchFullLoadCodecContextPtr  = std::unique_ptr<AVCodecContext, CodecContextDeleter>;
    using BatchFullLoadFramePtr         = std::unique_ptr<AVFrame, FrameDeleter>;
    using BatchFullLoadPacketPtr        = std::unique_ptr<AVPacket, PacketDeleter>;
    using BatchFullLoadSwrContextPtr    = std::unique_ptr<SwrContext, SwrContextDeleter>;
    inline BatchFullLoadFramePtr make_frame() { return BatchFullLoadFramePtr(av_frame_alloc()); }
    inline BatchFullLoadPacketPtr make_packet() { return BatchFullLoadPacketPtr(av_packet_alloc()); }
    inline BatchFullLoadCodecContextPtr make_codec_context(const AVCodec* codec) {
        return BatchFullLoadCodecContextPtr(avcodec_alloc_context3(codec));
    }
}