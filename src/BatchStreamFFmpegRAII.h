#pragma once

#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

namespace BatchStreamFFmpeg {
    struct FormatContextDeleter {void operator()(AVFormatContext* ctx) const {if (ctx) avformat_close_input(&ctx);}};
    struct CodecContextDeleter {void operator()(AVCodecContext* ctx) const {if (ctx) avcodec_free_context(&ctx);}};
    struct FrameDeleter {void operator()(AVFrame* frame) const {if (frame) av_frame_free(&frame);}};
    struct PacketDeleter {void operator()(AVPacket* pkt) const {if (pkt) av_packet_free(&pkt);}};
    struct SwrContextDeleter {void operator()(SwrContext* swr) const {if (swr) swr_free(&swr);}};
    using FormatContextPtr = std::unique_ptr<AVFormatContext, FormatContextDeleter>;
    using CodecContextPtr  = std::unique_ptr<AVCodecContext, CodecContextDeleter>;
    using FramePtr         = std::unique_ptr<AVFrame, FrameDeleter>;
    using PacketPtr        = std::unique_ptr<AVPacket, PacketDeleter>;
    using SwrContextPtr    = std::unique_ptr<SwrContext, SwrContextDeleter>;
}