#pragma once

#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}


namespace ffmpeg {

    struct FormatContextDeleter {void operator()(AVFormatContext* ctx) const noexcept {if (ctx) avformat_close_input(&ctx);}};
    struct CodecContextDeleter  {void operator()(AVCodecContext* ctx)  const noexcept {if (ctx) avcodec_free_context(&ctx);}};
    struct FrameDeleter         {void operator()(AVFrame* frame)       const noexcept {if (frame) av_frame_free(&frame)   ;}};
    struct PacketDeleter        {void operator()(AVPacket* pkt)        const noexcept {if (pkt) av_packet_free(&pkt)      ;}};
    struct SwrContextDeleter    {void operator()(SwrContext* swr)      const noexcept {if (swr) swr_free(&swr)            ;}};

    using FormatContextPtr = std::unique_ptr<AVFormatContext, FormatContextDeleter>;
    using CodecContextPtr  = std::unique_ptr<AVCodecContext, CodecContextDeleter>;
    using FramePtr         = std::unique_ptr<AVFrame, FrameDeleter>;
    using PacketPtr        = std::unique_ptr<AVPacket, PacketDeleter>;
    using SwrContextPtr    = std::unique_ptr<SwrContext, SwrContextDeleter>;

    inline FramePtr make_frame()   {return FramePtr(av_frame_alloc());}
    inline PacketPtr make_packet() {return PacketPtr(av_packet_alloc());}
    inline CodecContextPtr make_codec_context(const AVCodec* codec) {return CodecContextPtr(avcodec_alloc_context3(codec));}

} // namespace ffmpeg