#include "BatchStreamDecoder.h"
#include "AlignedAllocator.h" 

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#include <algorithm>

static int read_packet_from_stream(void* opaque, uint8_t* buf, int buf_size) {
    BatchStreamBuffer* sb = static_cast<BatchStreamBuffer*>(opaque);
    if (!sb) return AVERROR_EOF;
    int bytesRead = sb->read(buf, buf_size);
    if (bytesRead < 0) {
        return AVERROR(EIO);
    }
    if (bytesRead == 0) {
        return AVERROR_EOF;
    }
    return bytesRead;
}

BatchStreamDecoder::BatchStreamDecoder() {}

BatchStreamDecoder::~BatchStreamDecoder() {
    close();
}

void BatchStreamDecoder::close() {
    m_swrCtx.reset();
    m_codecCtx.reset();
    m_formatCtx.reset();
    m_frame.reset();
    m_packet.reset();
    if (m_avioCtx) {
        if (m_avioCtx->buffer) {
            av_freep(&m_avioCtx->buffer);
        }
        avio_context_free(&m_avioCtx);
        m_avioCtx = nullptr;
    }
    m_avioBuffer = nullptr;
    m_residueBuffer32.clear();
    m_residueBuffer64.clear();
    m_eofReached = false;
    m_decoderFlushed = false;
    m_useDoublePrecision = false;
}

bool BatchStreamDecoder::open(const QString& filePath, BatchStreamAudioInfo& info) {
    close(); 
    AVFormatContext* formatCtxRaw = nullptr;
    if (avformat_open_input(&formatCtxRaw, filePath.toUtf8().constData(), nullptr, nullptr) != 0) {
        return false;
    }
    m_formatCtx.reset(formatCtxRaw);
    return setupDecoder(filePath, info);
}

bool BatchStreamDecoder::open(std::shared_ptr<BatchStreamBuffer> buffer, const QString& filePathHint, BatchStreamAudioInfo& info) {
    close();
    if (!buffer) return false;
    const int ioBufferSize = 32768;
    m_avioBuffer = (unsigned char*)av_malloc(ioBufferSize);
    if (!m_avioBuffer) return false;
    m_avioCtx = avio_alloc_context(
        m_avioBuffer, ioBufferSize,
        0,
        buffer.get(),
        read_packet_from_stream,
        nullptr,
        nullptr
    );
    if (!m_avioCtx) {
        av_free(m_avioBuffer);
        return false;
    }
    AVFormatContext* formatCtxRaw = avformat_alloc_context();
    if (!formatCtxRaw) {
        avio_context_free(&m_avioCtx);
        return false;
    }
    formatCtxRaw->pb = m_avioCtx;
    formatCtxRaw->flags |= AVFMT_FLAG_CUSTOM_IO;
    if (avformat_open_input(&formatCtxRaw, filePathHint.toUtf8().constData(), nullptr, nullptr) != 0) {
        avformat_free_context(formatCtxRaw); 
        return false;
    }
    m_formatCtx.reset(formatCtxRaw);
    return setupDecoder(filePathHint, info);
}

bool BatchStreamDecoder::setupDecoder(const QString& filePath, BatchStreamAudioInfo& info) {
    if (avformat_find_stream_info(m_formatCtx.get(), nullptr) < 0) return false;
    m_audioStreamIndex = av_find_best_stream(m_formatCtx.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (m_audioStreamIndex < 0) return false;
    AVStream* stream = m_formatCtx->streams[m_audioStreamIndex];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) return false;
    AVCodecContext* codecCtxRaw = avcodec_alloc_context3(codec);
    if (!codecCtxRaw) return false;
    m_codecCtx.reset(codecCtxRaw);
    if (avcodec_parameters_to_context(m_codecCtx.get(), stream->codecpar) < 0) return false;
    m_codecCtx->thread_count = 1; 
    if (avcodec_open2(m_codecCtx.get(), codec, nullptr) < 0) return false;
    info.filePath = filePath;
    info.sampleRate = m_codecCtx->sample_rate;
    info.channels = m_codecCtx->ch_layout.nb_channels;
    int bits = stream->codecpar->bits_per_raw_sample;
    if (bits <= 0) bits = av_get_bytes_per_sample(m_codecCtx->sample_fmt) * 8;
    if (bits <= 0) bits = 16;
    info.sourceBitDepth = bits;
    m_useDoublePrecision = (bits > 32) || 
                           (m_codecCtx->sample_fmt == AV_SAMPLE_FMT_DBL) || 
                           (m_codecCtx->sample_fmt == AV_SAMPLE_FMT_DBLP);
    AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_MONO;
    AVSampleFormat outFmt = m_useDoublePrecision ? AV_SAMPLE_FMT_DBL : AV_SAMPLE_FMT_FLT;
    SwrContext* swrCtxRaw = nullptr;
    swr_alloc_set_opts2(&swrCtxRaw,
        &outLayout, outFmt, m_codecCtx->sample_rate,
        &m_codecCtx->ch_layout, m_codecCtx->sample_fmt, m_codecCtx->sample_rate,
        0, nullptr);
    m_swrCtx.reset(swrCtxRaw);
    if (!m_swrCtx || swr_init(m_swrCtx.get()) < 0) return false;
    m_frame.reset(av_frame_alloc());
    m_packet.reset(av_packet_alloc());
    int64_t durationMs = 0;
    bool durationFound = false;
    bool isSeekable = (m_avioCtx == nullptr); 
    if (isSeekable && (codec->id == AV_CODEC_ID_AAC || codec->id == AV_CODEC_ID_MP3)) {
        int64_t hugeTimestamp = (stream->duration != AV_NOPTS_VALUE) ? stream->duration : (1LL << 60);
        if (av_seek_frame(m_formatCtx.get(), m_audioStreamIndex, hugeTimestamp, AVSEEK_FLAG_BACKWARD) >= 0) {
            BatchStreamFFmpeg::PacketPtr tempPkt(av_packet_alloc());
            int64_t maxPts = 0;
            bool readAny = false;
            while (av_read_frame(m_formatCtx.get(), tempPkt.get()) >= 0) {
                if (tempPkt->stream_index == m_audioStreamIndex && tempPkt->pts != AV_NOPTS_VALUE) {
                    int64_t endPts = tempPkt->pts + tempPkt->duration;
                    if (endPts > maxPts) maxPts = endPts;
                    readAny = true;
                }
                av_packet_unref(tempPkt.get());
            }
            if (readAny && maxPts > 0) {
                durationMs = static_cast<int64_t>(maxPts * av_q2d(stream->time_base) * 1000);
                durationFound = true;
            }
        }
        av_seek_frame(m_formatCtx.get(), m_audioStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(m_codecCtx.get());
    }
    if (!durationFound) {
        if (stream->duration != AV_NOPTS_VALUE) {
            durationMs = static_cast<int64_t>(stream->duration * av_q2d(stream->time_base) * 1000);
        } else if (m_formatCtx->duration != AV_NOPTS_VALUE) {
            durationMs = m_formatCtx->duration / 1000;
        }
    }
    info.durationSec = durationMs / 1000.0;
    info.totalSamplesEst = static_cast<long long>(info.durationSec * info.sampleRate);
    return true;
}

BatchStreamPcmVariant BatchStreamDecoder::readNextChunk(size_t samplesToRead) {
    if (!m_formatCtx || (m_eofReached && m_decoderFlushed)) {
        if (m_useDoublePrecision) return BatchStreamPcm64();
        else return BatchStreamPcm32();
    }
    BatchStreamPcm32 out32;
    BatchStreamPcm64 out64;
    size_t collected = 0;
    if (m_useDoublePrecision) {
        if (!m_residueBuffer64.empty()) {
            size_t take = std::min(samplesToRead, m_residueBuffer64.size());
            out64.insert(out64.end(), m_residueBuffer64.begin(), m_residueBuffer64.begin() + take);
            m_residueBuffer64.erase(m_residueBuffer64.begin(), m_residueBuffer64.begin() + take);
            collected += take;
        }
    } else {
        if (!m_residueBuffer32.empty()) {
            size_t take = std::min(samplesToRead, m_residueBuffer32.size());
            out32.insert(out32.end(), m_residueBuffer32.begin(), m_residueBuffer32.begin() + take);
            m_residueBuffer32.erase(m_residueBuffer32.begin(), m_residueBuffer32.begin() + take);
            collected += take;
        }
    }
    while (collected < samplesToRead) {
        int rxRet = avcodec_receive_frame(m_codecCtx.get(), m_frame.get());
        if (rxRet == 0) {
            int max_out = swr_get_out_samples(m_swrCtx.get(), m_frame->nb_samples);
            if (max_out > 0) {
                if (m_useDoublePrecision) {
                    std::vector<double, AlignedAllocator<double, 64>> buf(max_out);
                    uint8_t* outData[1] = { reinterpret_cast<uint8_t*>(buf.data()) };
                    int outSamples = swr_convert(m_swrCtx.get(), outData, max_out, (const uint8_t**)m_frame->data, m_frame->nb_samples);
                    if (outSamples > 0) {
                        size_t needed = samplesToRead - collected;
                        size_t toCopy = std::min((size_t)outSamples, needed);
                        out64.insert(out64.end(), buf.begin(), buf.begin() + toCopy);
                        collected += toCopy;
                        if (toCopy < (size_t)outSamples) {
                            m_residueBuffer64.insert(m_residueBuffer64.end(), buf.begin() + toCopy, buf.begin() + outSamples);
                        }
                    }
                } else {
                    std::vector<float, AlignedAllocator<float, 64>> buf(max_out);
                    uint8_t* outData[1] = { reinterpret_cast<uint8_t*>(buf.data()) };
                    int outSamples = swr_convert(m_swrCtx.get(), outData, max_out, (const uint8_t**)m_frame->data, m_frame->nb_samples);
                    if (outSamples > 0) {
                        size_t needed = samplesToRead - collected;
                        size_t toCopy = std::min((size_t)outSamples, needed);
                        out32.insert(out32.end(), buf.begin(), buf.begin() + toCopy);
                        collected += toCopy;
                        if (toCopy < (size_t)outSamples) {
                            m_residueBuffer32.insert(m_residueBuffer32.end(), buf.begin() + toCopy, buf.begin() + outSamples);
                        }
                    }
                }
            }
            av_frame_unref(m_frame.get());
            continue; 
        }
        else if (rxRet == AVERROR_EOF) {
            m_decoderFlushed = true;
            break; 
        }
        else if (rxRet != AVERROR(EAGAIN)) {
            continue; 
        }
        if (m_eofReached) {
            if (!m_decoderFlushed) {
                avcodec_send_packet(m_codecCtx.get(), nullptr);
            } else {
                break;
            }
            continue;
        }
        int readRet = av_read_frame(m_formatCtx.get(), m_packet.get());
        if (readRet < 0) {
            if (readRet == AVERROR_EOF) {
                m_eofReached = true;
                avcodec_send_packet(m_codecCtx.get(), nullptr);
            }
            continue;
        }
        if (m_packet->stream_index == m_audioStreamIndex) {
            avcodec_send_packet(m_codecCtx.get(), m_packet.get());
        }
        av_packet_unref(m_packet.get());
    }
    if (m_decoderFlushed && collected < samplesToRead) {
        int delayed = swr_get_delay(m_swrCtx.get(), m_codecCtx->sample_rate);
        if (delayed > 0) {
             if (m_useDoublePrecision) {
                std::vector<double, AlignedAllocator<double, 64>> buf(delayed);
                uint8_t* outData[1] = { reinterpret_cast<uint8_t*>(buf.data()) };
                int outSamples = swr_convert(m_swrCtx.get(), outData, delayed, nullptr, 0);
                if(outSamples > 0) out64.insert(out64.end(), buf.begin(), buf.begin() + outSamples);
             } else {
                std::vector<float, AlignedAllocator<float, 64>> buf(delayed);
                uint8_t* outData[1] = { reinterpret_cast<uint8_t*>(buf.data()) };
                int outSamples = swr_convert(m_swrCtx.get(), outData, delayed, nullptr, 0);
                if(outSamples > 0) out32.insert(out32.end(), buf.begin(), buf.begin() + outSamples);
             }
        }
        swr_init(m_swrCtx.get()); 
    }
    if (m_useDoublePrecision) return out64;
    else return out32;
}