#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include <memory>

namespace BatFFmpeg {
    struct FormatContextDeleter {void operator()(AVFormatContext* ctx) const noexcept {if (ctx) avformat_close_input(&ctx);}};
    struct CodecContextDeleter {void operator()(AVCodecContext* ctx) const noexcept {if (ctx) avcodec_free_context(&ctx);}};
    struct FrameDeleter {void operator()(AVFrame* frame) const noexcept {if (frame) av_frame_free(&frame);}};
    struct PacketDeleter {void operator()(AVPacket* pkt) const noexcept {if (pkt) av_packet_free(&pkt);}};
    struct SwrContextDeleter {void operator()(SwrContext* swr) const noexcept {if (swr) swr_free(&swr);}};
    using BatFormatContextPtr = std::unique_ptr<AVFormatContext, FormatContextDeleter>;
    using BatCodecContextPtr  = std::unique_ptr<AVCodecContext, CodecContextDeleter>;
    using BatFramePtr         = std::unique_ptr<AVFrame, FrameDeleter>;
    using BatPacketPtr        = std::unique_ptr<AVPacket, PacketDeleter>;
    using BatSwrContextPtr    = std::unique_ptr<SwrContext, SwrContextDeleter>;
    inline BatFramePtr make_frame() { return BatFramePtr(av_frame_alloc()); }
    inline BatPacketPtr make_packet() { return BatPacketPtr(av_packet_alloc()); }
    inline BatCodecContextPtr make_codec_context(const AVCodec* codec) {
        return BatCodecContextPtr(avcodec_alloc_context3(codec));
    }
}