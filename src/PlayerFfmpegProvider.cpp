#include "PlayerFfmpegProvider.h"

#include <algorithm>
#include <cstring> 
#include <cmath>
#include <cstdio>
#include <vector>

class IOBufferStation {
public:
    IOBufferStation(const QString& path, size_t capacity) 
        : m_capacity(capacity) 
    {
#ifdef _WIN32
        m_fp = _wfopen(path.toStdWString().c_str(), L"rb");
#else
        m_fp = fopen(path.toUtf8().constData(), "rb");
#endif
        if (m_fp) {
            fseek(m_fp, 0, SEEK_END);
#ifdef _WIN32
            m_fileSize = _ftelli64(m_fp);
#else
            m_fileSize = ftello(m_fp);
#endif
            fseek(m_fp, 0, SEEK_SET);
            m_buffer.resize(m_capacity);
            reloadBuffer(0);
            if (m_fileSize <= static_cast<int64_t>(m_capacity)) {
                fclose(m_fp);
                m_fp = nullptr;
                m_isFullyLoaded = true;
            }
        }
    }
    ~IOBufferStation() {
        if (m_fp) fclose(m_fp);
    }
    bool isOpen() const { return m_fp != nullptr || m_isFullyLoaded; }
    int read(uint8_t* buf, int size) {
        std::lock_guard<std::mutex> lock(m_ioMutex);
        if (!m_fp && !m_isFullyLoaded) return -1;
        if (m_cursor >= m_validLength) {
            int64_t nextFilePos = m_fileStartOffset + m_validLength;
            if (nextFilePos < m_fileSize) {
                if (m_fp) reloadBuffer(nextFilePos);
                else return AVERROR_EOF;
            } else {
                return AVERROR_EOF;
            }
        }
        size_t available = m_validLength - m_cursor;
        int toRead = std::min(static_cast<int>(available), size);
        if (toRead > 0) {
            std::memcpy(buf, m_buffer.data() + m_cursor, toRead);
            m_cursor += toRead;
        }
        if (m_fp && m_cursor > m_validLength * 0.75) {
            checkAndRefill();
        }
        return toRead;
    }
    int64_t seek(int64_t offset, int whence) {
        std::lock_guard<std::mutex> lock(m_ioMutex);
        if (!m_fp && !m_isFullyLoaded) return -1;
        if (whence == AVSEEK_SIZE) return m_fileSize;
        int64_t targetPos = -1;
        if (whence == SEEK_SET) targetPos = offset;
        else if (whence == SEEK_CUR) targetPos = (m_fileStartOffset + m_cursor) + offset;
        else if (whence == SEEK_END) targetPos = m_fileSize + offset;
        if (targetPos < 0 || targetPos > m_fileSize) return -1;
        int64_t localOffset = targetPos - m_fileStartOffset;
        if (localOffset >= 0 && localOffset <= static_cast<int64_t>(m_validLength)) {
            m_cursor = static_cast<size_t>(localOffset);
        } else {
            if (m_fp) reloadBuffer(targetPos);
            else return -1;
        }
        return targetPos;
    }
private:
    FILE* m_fp = nullptr;
    bool m_isFullyLoaded = false;
    std::vector<uint8_t> m_buffer;
    size_t m_capacity = 0;
    int64_t m_fileSize = 0;
    int64_t m_fileStartOffset = 0;
    size_t m_validLength = 0;
    size_t m_cursor = 0;
    std::mutex m_ioMutex;
    void reloadBuffer(int64_t filePos) {
        if (!m_fp) return;
#ifdef _WIN32
        _fseeki64(m_fp, filePos, SEEK_SET);
#else
        fseeko(m_fp, filePos, SEEK_SET);
#endif
        size_t readBytes = fread(m_buffer.data(), 1, m_capacity, m_fp);
        m_fileStartOffset = filePos;
        m_validLength = readBytes;
        m_cursor = 0;
    }
    void checkAndRefill() {
        if (m_fileStartOffset + m_validLength >= m_fileSize) return;
        size_t remaining = m_validLength - m_cursor;
        if (remaining > 0) {
            std::memmove(m_buffer.data(), m_buffer.data() + m_cursor, remaining);
        }
        int64_t readStartPos = m_fileStartOffset + m_validLength;
#ifdef _WIN32
        _fseeki64(m_fp, readStartPos, SEEK_SET);
#else
        fseeko(m_fp, readStartPos, SEEK_SET);
#endif
        size_t spaceLeft = m_capacity - remaining;
        size_t readBytes = fread(m_buffer.data() + remaining, 1, spaceLeft, m_fp);
        m_fileStartOffset += m_cursor;
        m_validLength = remaining + readBytes;
        m_cursor = 0;
    }
};

static int io_read_packet(void *opaque, uint8_t *buf, int buf_size) {
    IOBufferStation* station = static_cast<IOBufferStation*>(opaque);
    return station->read(buf, buf_size);
}

static int64_t io_seek_packet(void *opaque, int64_t offset, int whence) {
    IOBufferStation* station = static_cast<IOBufferStation*>(opaque);
    return station->seek(offset, whence);
}

PlayerFfmpegProvider::PlayerFfmpegProvider(const QString& filePath, int trackIndex, int targetChannelIdx, 
                                           double startSec, double endSec, size_t bufferCapacity) 
    : m_targetChannelIdx(targetChannelIdx), m_startSec(startSec), m_endSec(endSec)
{
    m_isRangeMode = (m_startSec > 0.001 || m_endSec > 0.001);
    m_ioStation = std::make_unique<IOBufferStation>(filePath, bufferCapacity);
    if (!m_ioStation->isOpen()) return;
    const int avioBufferSize = 32768;
    m_avioBuffer = (unsigned char*)av_malloc(avioBufferSize);
    if (!m_avioBuffer) return;
    m_avioCtx = avio_alloc_context(
        m_avioBuffer, avioBufferSize,
        0, 
        m_ioStation.get(),
        io_read_packet, 
        nullptr, 
        io_seek_packet
    );
    if (!m_avioCtx) {
        av_free(m_avioBuffer);
        m_avioBuffer = nullptr;
        return;
    }
    m_formatCtx.reset(avformat_alloc_context());
    m_formatCtx->pb = m_avioCtx;
    m_formatCtx->flags |= AVFMT_FLAG_CUSTOM_IO; 
    AVFormatContext* ctxRaw = m_formatCtx.release();
    if (avformat_open_input(&ctxRaw, nullptr, nullptr, nullptr) != 0) {
        if (m_avioCtx) {
            av_freep(&m_avioCtx->buffer);
            avio_context_free(&m_avioCtx);
        }
        return; 
    }
    m_formatCtx.reset(ctxRaw);
    if (avformat_find_stream_info(m_formatCtx.get(), nullptr) < 0) return;
    if (trackIndex < 0) {
        m_streamIndex = av_find_best_stream(m_formatCtx.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    } else {
        if (trackIndex < static_cast<int>(m_formatCtx->nb_streams) &&
            m_formatCtx->streams[trackIndex]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_streamIndex = trackIndex;
        }
    }
    if (m_streamIndex < 0) return;
    AVStream* stream = m_formatCtx->streams[m_streamIndex];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) return;
    AVCodecContext* codecCtxRaw = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtxRaw, stream->codecpar);
    codecCtxRaw->thread_count = 0; 
    if (avcodec_open2(codecCtxRaw, codec, nullptr) < 0) {
        avcodec_free_context(&codecCtxRaw);
        return;
    }
    m_codecCtx.reset(codecCtxRaw);
    m_packet.reset(av_packet_alloc());
    m_frame.reset(av_frame_alloc());
    m_timeBase = av_q2d(stream->time_base);
    double physDuration = 0.0;
    if (m_formatCtx->duration != AV_NOPTS_VALUE) {
        physDuration = static_cast<double>(m_formatCtx->duration) / AV_TIME_BASE;
    } else {
        physDuration = static_cast<double>(stream->duration) * m_timeBase;
    }
    if (m_isRangeMode) {
        double effectiveEnd = (m_endSec > 0.001) ? m_endSec : physDuration;
        m_duration = effectiveEnd - m_startSec;
        if (m_duration < 0) m_duration = 0;
    } else {
        m_duration = physDuration;
    }
    int srcRate = m_codecCtx->sample_rate;
    int srcChannels = m_codecCtx->ch_layout.nb_channels;
    AVSampleFormat srcFmt = m_codecCtx->sample_fmt;
    m_targetRate = srcRate; 
    if (!m_resampler.init(srcRate, srcChannels, srcFmt, srcRate, m_targetChannelIdx)) return;
    m_isValid = true;
    if (m_isRangeMode && m_startSec > 0.001) {
        seek(0.0);
    }
}

PlayerFfmpegProvider::~PlayerFfmpegProvider() {
    m_codecCtx.reset();
    m_formatCtx.reset();
    if (m_avioCtx) {
        av_freep(&m_avioCtx->buffer);
        avio_context_free(&m_avioCtx);
    }
    m_ioStation.reset();
}

int PlayerFfmpegProvider::getNativeSampleRate() const {
    if (m_codecCtx) return m_codecCtx->sample_rate;
    return 44100;
}

void PlayerFfmpegProvider::configureResampler(int targetRate) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_codecCtx) return;
    int srcRate = m_codecCtx->sample_rate;
    int srcChannels = m_codecCtx->ch_layout.nb_channels;
    AVSampleFormat srcFmt = m_codecCtx->sample_fmt;
    m_targetRate = targetRate;
    m_resampler.init(srcRate, srcChannels, srcFmt, targetRate, m_targetChannelIdx);
    m_sampleBuffer.clear();
    m_bufferOffset = 0;
}

bool PlayerFfmpegProvider::decodeNextFrame() {
    if (m_isEof) return false;
    while (true) {
        int ret = avcodec_receive_frame(m_codecCtx.get(), m_frame.get());
        if (ret == 0) {
            double framePtsTime = 0.0;
            if (m_frame->pts != AV_NOPTS_VALUE) {
                framePtsTime = m_frame->pts * m_timeBase;
            } else {
                framePtsTime = m_lastDecodedPtsEnd;
            }
            if (m_isFirstFrameAfterSeek) {
                m_lastDecodedPtsEnd = framePtsTime;
                m_isFirstFrameAfterSeek = false;
            }
            double frameDuration = (double)m_frame->nb_samples / m_codecCtx->sample_rate;
            double frameEndTime = framePtsTime + frameDuration;
            m_lastDecodedPtsEnd = std::max(m_lastDecodedPtsEnd, frameEndTime);
            if (m_isRangeMode && m_endSec > 0.001 && framePtsTime >= m_endSec) {
                m_isEof = true;
                av_frame_unref(m_frame.get());
                return false;
            }
            int skipSamples = 0;
            int validSamples = m_frame->nb_samples;
            if (m_isRangeMode) {
                if (framePtsTime < m_startSec) {
                    double diff = m_startSec - framePtsTime;
                    skipSamples = static_cast<int>(diff * m_codecCtx->sample_rate);
                    if (skipSamples >= m_frame->nb_samples) {
                        av_frame_unref(m_frame.get());
                        continue;
                    }
                    validSamples -= skipSamples;
                }
                if (m_endSec > 0.001 && frameEndTime > m_endSec) {
                    double keepDuration = m_endSec - (framePtsTime + (double)skipSamples / m_codecCtx->sample_rate);
                    int keep = static_cast<int>(keepDuration * m_codecCtx->sample_rate);
                    if (keep < 0) keep = 0;
                    if (keep < validSamples) validSamples = keep;
                    m_isEof = true;
                }
            }
            if (validSamples > 0) {
                const uint8_t** srcPlanar = const_cast<const uint8_t**>(m_frame->data);
                std::vector<float> resampled = m_resampler.process(srcPlanar, m_frame->nb_samples);
                size_t totalOut = resampled.size();
                size_t channels = m_resampler.getOutputChannels();
                size_t samplesPerCh = (channels > 0) ? (totalOut / channels) : 0;
                size_t outSkip = 0;
                size_t outKeep = totalOut;
                if (m_frame->nb_samples > 0 && samplesPerCh > 0) {
                    double ratio = (double)samplesPerCh / m_frame->nb_samples;
                    size_t skipFrames = static_cast<size_t>(skipSamples * ratio);
                    size_t validFrames = static_cast<size_t>(validSamples * ratio);
                    outSkip = skipFrames * channels;
                    outKeep = validFrames * channels;
                }
                if (outSkip < totalOut) {
                    size_t actualCopy = std::min(outKeep, totalOut - outSkip);
                    if (actualCopy > 0) {
                        m_sampleBuffer.insert(m_sampleBuffer.end(), 
                                              resampled.begin() + outSkip, 
                                              resampled.begin() + outSkip + actualCopy);
                    }
                }
            }
            av_frame_unref(m_frame.get());
            return true;
        }
        else if (ret == AVERROR_EOF) {
            m_isEof = true;
            return false;
        }
        else if (ret < 0 && ret != AVERROR(EAGAIN)) {
            return false;
        }
        if (m_isReadEof) {
            avcodec_send_packet(m_codecCtx.get(), nullptr);
            continue;
        }
        int readRet = av_read_frame(m_formatCtx.get(), m_packet.get());
        if (readRet < 0) {
            m_isReadEof = true;
            avcodec_send_packet(m_codecCtx.get(), nullptr);
            continue;
        }
        if (m_packet->stream_index == m_streamIndex) {
            if (avcodec_send_packet(m_codecCtx.get(), m_packet.get()) == 0) {
                av_packet_unref(m_packet.get());
                continue; 
            }
        }
        av_packet_unref(m_packet.get());
    }
}

qint64 PlayerFfmpegProvider::readSamples(float* buffer, qint64 maxFloatCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isValid) return 0;
    qint64 totalCopied = 0;
    while (totalCopied < maxFloatCount) {
        if (m_bufferOffset < m_sampleBuffer.size()) {
            size_t available = m_sampleBuffer.size() - m_bufferOffset;
            size_t toCopy = std::min((size_t)(maxFloatCount - totalCopied), available);
            std::memcpy(buffer + totalCopied, m_sampleBuffer.data() + m_bufferOffset, toCopy * sizeof(float));
            m_bufferOffset += toCopy;
            totalCopied += toCopy;
            if (m_bufferOffset >= m_sampleBuffer.size()) {
                m_sampleBuffer.clear();
                m_bufferOffset = 0;
            }
        } 
        else {
            if (!decodeNextFrame()) {
                break;
            }
        }
    }
    return totalCopied;
}

void PlayerFfmpegProvider::seek(double seconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isValid) return;
    double targetPhysical = seconds + m_startSec;
    if (m_isRangeMode && m_endSec > 0.001 && targetPhysical > m_endSec) {
        targetPhysical = m_endSec;
    }
    int64_t timestamp = static_cast<int64_t>(targetPhysical / m_timeBase);
    avcodec_flush_buffers(m_codecCtx.get());
    if (av_seek_frame(m_formatCtx.get(), m_streamIndex, timestamp, AVSEEK_FLAG_BACKWARD) < 0) {}
    m_sampleBuffer.clear();
    m_bufferOffset = 0;
    m_resampler.reset();
    m_isEof = false;
    m_isReadEof = false;
    m_lastDecodedPtsEnd = targetPhysical; 
    m_isFirstFrameAfterSeek = true;
}

double PlayerFfmpegProvider::currentTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    double rel = m_lastDecodedPtsEnd - m_startSec;
    if (rel < 0) rel = 0;
    return rel;
}

bool PlayerFfmpegProvider::atEnd() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isEof && (m_bufferOffset >= m_sampleBuffer.size());
}

double PlayerFfmpegProvider::getLastDecodedTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    double rel = m_lastDecodedPtsEnd - m_startSec;
    if (rel < 0) rel = 0;
    return rel;
}

double PlayerFfmpegProvider::getInternalBufferDuration() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_targetRate <= 0) return 0.0;
    int channels = m_resampler.getOutputChannels();
    if (channels <= 0) return 0.0;
    size_t remainingFloats = 0;
    if (m_bufferOffset < m_sampleBuffer.size()) {
        remainingFloats = m_sampleBuffer.size() - m_bufferOffset;
    }
    return static_cast<double>(remainingFloats) / (m_targetRate * channels);
}