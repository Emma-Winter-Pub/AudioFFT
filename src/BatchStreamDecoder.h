#pragma once

#include "BatchStreamTypes.h"
#include "BatchStreamFFmpegRAII.h"
#include "BatchStreamBuffer.h" 

#include <memory>
#include <atomic>

class BatchStreamDecoder {
public:
    BatchStreamDecoder();
    ~BatchStreamDecoder();
    bool open(const QString& filePath, BatchStreamAudioInfo& info);
    bool open(std::shared_ptr<BatchStreamBuffer> buffer, const QString& filePathHint, BatchStreamAudioInfo& info, int codecIdHint = 0);
    BatchStreamPcmVariant readNextChunk(size_t samplesToRead);
    void close();

private:
    bool setupDecoder(const QString& filePath, BatchStreamAudioInfo& info);
    BatchStreamFFmpegRAII::FormatContextPtr m_formatCtx;
    BatchStreamFFmpegRAII::CodecContextPtr  m_codecCtx;
    BatchStreamFFmpegRAII::SwrContextPtr    m_swrCtx;
    BatchStreamFFmpegRAII::FramePtr         m_frame;
    BatchStreamFFmpegRAII::PacketPtr        m_packet;
    unsigned char* m_avioBuffer = nullptr;
    AVIOContext*   m_avioCtx = nullptr;
    int m_audioStreamIndex = -1;
    bool m_useDoublePrecision = false;
    bool m_eofReached = false;
    bool m_decoderFlushed = false;
    BatchStreamPcm32 m_residueBuffer32;
    BatchStreamPcm64 m_residueBuffer64;
};