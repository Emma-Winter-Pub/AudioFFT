#pragma once

#include <vector>
#include <string>
#include <QString>
#include <QList>
#include <QStringList>
#include <functional>
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

namespace DecoderTypes {
    struct AVFormatContextDeleter { void operator()(AVFormatContext* ctx) const { if (ctx)avformat_close_input(&ctx); } };
    struct AVCodecContextDeleter  { void operator()(AVCodecContext* ctx)  const { if (ctx)avcodec_free_context(&ctx); } };
    struct AVFrameDeleter         { void operator()(AVFrame* frame)       const { if (frame)av_frame_free(&frame);    } };
    struct AVPacketDeleter        { void operator()(AVPacket* pkt)        const { if (pkt)av_packet_free(&pkt);       } };
    struct SwrContextDeleter      { void operator()(SwrContext* swr)      const { if (swr)swr_free(&swr);             } };
    using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
    using AVCodecContextPtr  = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
    using AVFramePtr         = std::unique_ptr<AVFrame, AVFrameDeleter>;
    using AVPacketPtr        = std::unique_ptr<AVPacket, AVPacketDeleter>;
    using SwrContextPtr      = std::unique_ptr<SwrContext, SwrContextDeleter>;
    struct AudioTrack { int index = 0; QString name; int channels = 0; QString channelLayout; QStringList channelNames; };
    struct AudioMetadata {
        QString filePath;
        QString formatName;
        QString codecName;
        double durationSec = 0.0;
        int sampleRate = 0;
        int channels = 0;
        long long bitRate = 0;
        long long preciseDurationMicroseconds = 0; 
        int sourceBitDepth = 0; 
        QList<AudioTrack> tracks;
        int currentTrackIndex = -1;
        int defaultTrackIndex = -1;
    };
    using LogCallback = std::function<void(const QString&)>;
}