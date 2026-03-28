#include "FastAudioDecoder.h"
#include "FastUtils.h"

#include <QFileInfo>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <QTime>

extern "C" {
#include <libavutil/opt.h>
}

static QString formatDuration(double total_seconds) {
    long long totalSec = static_cast<long long>(total_seconds);
    int hours = totalSec / 3600;
    int minutes = (totalSec % 3600) / 60;
    int seconds = totalSec % 60;
    if (hours > 0) {
        return FastAudioDecoder::tr("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return FastAudioDecoder::tr("%1:%2")
            .arg(minutes)
            .arg(seconds, 2, 10, QChar('0'));
    }
}

FastAudioDecoder::FastAudioDecoder(QObject* parent):QObject(parent){}

FastAudioDecoder::~FastAudioDecoder() {close();}

void FastAudioDecoder::resetState() {
    m_formatCtx.reset();
    m_codecCtx.reset();
    m_swrCtx.reset();
    m_packet.reset();
    m_frame.reset();
    m_metadata = FastTypes::FastAudioMetadata();
    m_streamIndex = -1;
    m_useDoublePrecision = false;
    m_eofReached = false;
    m_startSec = 0.0;
    m_endSec = 0.0;
    m_isRangeMode = false;
    m_nextExpectedTime = 0.0;
    m_timeBase = 0.0;
    m_tempCueSheet.reset();
}

void FastAudioDecoder::close() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    resetState();
}

QString FastAudioDecoder::getChannelLayoutString(const AVChannelLayout* ch_layout) {
    char buf[128];
    av_channel_layout_describe(ch_layout, buf, sizeof(buf));
    return QString::fromUtf8(buf);
}

void FastAudioDecoder::logFileDetails(AVFormatContext* formatContext, double totalDuration) {
    emit logMessage(tr("    文件信息："));
    emit logMessage(tr("        [路径] %1").arg(m_metadata.filePath));
    emit logMessage(tr("        [大小] %1").arg(FastUtils::formatSize(QFileInfo(m_metadata.filePath).size())));
    emit logMessage(tr("        [时长] %1").arg(formatDuration(totalDuration)));
    emit logMessage(tr("        [采样率] %1 Hz").arg(m_metadata.sampleRate));
    emit logMessage(tr("        [码率] %1 kb/s").arg(static_cast<qint64>(m_metadata.bitRate / 1000)));
    emit logMessage(tr("        [位深] %1 bit").arg(m_metadata.sourceBitDepth));
    emit logMessage(tr("        [编码] %1").arg(m_metadata.codecName));
    emit logMessage(tr("        [封装] %1").arg(m_metadata.formatName));
    for (const auto& trk : m_metadata.tracks) {
        QString currentMark = (trk.index == m_metadata.currentTrackIndex) ? "◀   " : "     ";
        emit logMessage(tr("        [音轨] %1 %2 %3 (%4) %5声道")
            .arg(trk.index)
            .arg(currentMark)
            .arg(trk.name)
            .arg(trk.channelLayout)
            .arg(trk.channels));
    }
    if (m_tempCueSheet.has_value()) {
        const auto& sheet = m_tempCueSheet.value();
        emit logMessage(tr("        [分轨]"));
        for (int i = 0; i < sheet.tracks.size(); ++i) {
            const auto& t = sheet.tracks[i];
            QString startStr = QTime::fromMSecsSinceStartOfDay(static_cast<int>(t.startSeconds * 1000)).toString("HH:mm:ss");
            QString endStr;
            double endSec = t.endSeconds;
            if (endSec <= 0.001 && i == sheet.tracks.size() - 1) {
                endSec = totalDuration;
            }
            if (endSec > 0.001) {
                endStr = QTime::fromMSecsSinceStartOfDay(static_cast<int>(endSec * 1000)).toString("HH:mm:ss");
            }
            else {
                endStr = "??:??:??";
            }
            QString performer = t.performer;
            if (performer.isEmpty()) performer = sheet.albumPerformer;

            emit logMessage(tr("                %1：%2 - %3  %4 - %5")
                .arg(t.number, 2, 10, QChar('0'))
                .arg(startStr)
                .arg(endStr)
                .arg(performer)
                .arg(t.title));
        }
    }
    emit logMessage(tr("    元数据："));
    const AVDictionaryEntry* tag = nullptr;
    auto log_tag = [this, &tag](const char* key, const QString& display_name, AVFormatContext* ctx) {
        tag = av_dict_get(ctx->metadata, key, nullptr, AV_DICT_IGNORE_SUFFIX);
        if (tag && strlen(tag->value) > 0)
            emit logMessage(tr("        [%1] %2").arg(display_name).arg(QString::fromUtf8(tag->value)));
        };
    log_tag("title", tr("标题"), formatContext);
    log_tag("artist", tr("歌手"), formatContext);
    log_tag("album", tr("专辑"), formatContext);
    log_tag("track", tr("分轨"), formatContext);
    log_tag("date", tr("日期"), formatContext);
}

bool FastAudioDecoder::buildChannelMatrix(SwrContext* swr, int inputChannels, int targetChannelIdx) {
    if (targetChannelIdx < 0 || targetChannelIdx >= inputChannels) {
        return true; 
    }
    std::vector<double> matrix(inputChannels, 0.0);
    matrix[targetChannelIdx] = 1.0;
    if (swr_set_matrix(swr, matrix.data(), 1) < 0) {
        emit logMessage(tr("        错误：设置声道提取矩阵失败"));
        return false;
    }
    return true;
}

bool FastAudioDecoder::open(const QString& filePath, int targetTrackIdx, int targetChannelIdx,
    double startSec, double endSec,
    std::optional<CueSheet> cueSheet)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    resetState();
    m_eofReached = false;
    m_startSec = startSec;
    m_endSec = endSec;
    m_isRangeMode = (m_startSec > 0.001 || m_endSec > 0.001);
    m_tempCueSheet = cueSheet;
    emit logMessage(FastUtils::getCurrentTimestamp() + " " + tr("探测文件"));
    AVFormatContext* formatCtxRaw = nullptr;
    if (avformat_open_input(&formatCtxRaw, filePath.toUtf8().constData(), nullptr, nullptr) != 0) {
        emit logMessage(tr("        错误：无法打开文件"));
        return false;
    }
    m_formatCtx = FastTypes::AVFormatContextPtr(formatCtxRaw);

    if (avformat_find_stream_info(m_formatCtx.get(), nullptr) < 0) {
        emit logMessage(tr("        错误：无法获取流信息"));
        return false;
    }
    int bestStreamIdx = -1;
    for (unsigned int i = 0; i < m_formatCtx->nb_streams; ++i) {
        AVStream* st = m_formatCtx->streams[i];
        if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (bestStreamIdx < 0) bestStreamIdx = av_find_best_stream(m_formatCtx.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
            FastTypes::FastAudioTrack track;
            track.index = i;
            track.channels = st->codecpar->ch_layout.nb_channels;
            track.channelLayout = getChannelLayoutString(&st->codecpar->ch_layout);
            for (int chIdx = 0; chIdx < track.channels; ++chIdx) {
                enum AVChannel chEnum = av_channel_layout_channel_from_index(&st->codecpar->ch_layout, chIdx);
                char nameBuf[32] = { 0 };
                av_channel_name(nameBuf, sizeof(nameBuf), chEnum);
                QString chName = QString::fromUtf8(nameBuf);
                if (chName.isEmpty()) chName = QString("%1").arg(chIdx + 1);
                track.channelNames.append(chName.toUpper());
            }
            const AVCodec* c = avcodec_find_decoder(st->codecpar->codec_id);
            QString codecStr = c ? c->name : "unknown";
            AVDictionaryEntry* tagLang = av_dict_get(st->metadata, "language", nullptr, 0);
            QString lang = tagLang ? tagLang->value : "";
            track.name = tr("音轨 %1: %2%3")
                .arg(i)
                .arg(codecStr.toUpper())
                .arg(lang.isEmpty() ? "" : " (" + lang + ")");
            m_metadata.tracks.append(track);
        }
    }
    if (m_metadata.tracks.isEmpty()) {
        emit logMessage(tr("        错误：未找到音频流"));
        return false;
    }
    m_metadata.defaultTrackIndex = (bestStreamIdx >= 0) ? bestStreamIdx : m_metadata.tracks[0].index;
    m_streamIndex = (targetTrackIdx < 0) ? m_metadata.defaultTrackIndex : targetTrackIdx;
    m_metadata.currentTrackIndex = m_streamIndex;
    AVStream* stream = m_formatCtx->streams[m_streamIndex];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        emit logMessage(tr("        错误：未找到解码器"));
        return false;
    }
    m_timeBase = av_q2d(stream->time_base);
    m_codecCtx = FastTypes::AVCodecContextPtr(avcodec_alloc_context3(codec));
    avcodec_parameters_to_context(m_codecCtx.get(), stream->codecpar);
    if (codec->capabilities & AV_CODEC_CAP_SLICE_THREADS) {
        m_codecCtx->thread_count = 0;
        m_codecCtx->thread_type = FF_THREAD_SLICE;
    }
    else if (codec->capabilities & AV_CODEC_CAP_FRAME_THREADS) {
        m_codecCtx->thread_count = 0;
        m_codecCtx->thread_type = FF_THREAD_FRAME;
    }
    else {
        m_codecCtx->thread_count = 1;
    }
    if (avcodec_open2(m_codecCtx.get(), codec, nullptr) < 0) {
        emit logMessage(tr("        错误：无法启动解码器"));
        return false;
    }
    double totalFileDuration = 0.0;
    if (m_formatCtx->duration != AV_NOPTS_VALUE) {
        totalFileDuration = m_formatCtx->duration / (double)AV_TIME_BASE;
    }
    else if (stream->duration != AV_NOPTS_VALUE) {
        totalFileDuration = stream->duration * m_timeBase;
    }
    if (codec->id == AV_CODEC_ID_AAC || codec->id == AV_CODEC_ID_MP3) {
        int64_t hugeTimestamp = (stream->duration != AV_NOPTS_VALUE) ? stream->duration : (1LL << 60);
        if (av_seek_frame(m_formatCtx.get(), m_streamIndex, hugeTimestamp, AVSEEK_FLAG_BACKWARD) >= 0) {
            FastTypes::AVPacketPtr tempPkt(av_packet_alloc());
            int64_t maxPts = 0;
            bool readAny = false;
            while (av_read_frame(m_formatCtx.get(), tempPkt.get()) >= 0) {
                if (tempPkt->stream_index == m_streamIndex && tempPkt->pts != AV_NOPTS_VALUE) {
                    int64_t endPts = tempPkt->pts + tempPkt->duration;
                    if (endPts > maxPts) maxPts = endPts;
                    readAny = true;
                }
                av_packet_unref(tempPkt.get());
            }
            if (readAny && maxPts > 0) {
                totalFileDuration = maxPts * m_timeBase;
            }
        }
        av_seek_frame(m_formatCtx.get(), m_streamIndex, 0, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(m_codecCtx.get());
    }
    double effectiveEnd = m_endSec;
    if (effectiveEnd <= 0.001) effectiveEnd = totalFileDuration;
    else if (totalFileDuration > 0.001 && effectiveEnd > totalFileDuration) effectiveEnd = totalFileDuration;
    if (m_endSec <= 0.001) m_endSec = totalFileDuration;
    double effectiveDuration = 0.0;
    if (m_isRangeMode) {
        effectiveDuration = (effectiveEnd > m_startSec) ? (effectiveEnd - m_startSec) : 0.0;
    }
    else {
        effectiveDuration = totalFileDuration;
    }
    m_metadata.filePath = filePath;
    m_metadata.formatName = m_formatCtx->iformat->long_name ? m_formatCtx->iformat->long_name : m_formatCtx->iformat->name;
    m_metadata.codecName = codec->long_name ? codec->long_name : codec->name;
    m_metadata.sampleRate = stream->codecpar->sample_rate;
    m_metadata.channels = stream->codecpar->ch_layout.nb_channels;
    m_metadata.durationSec = effectiveDuration;
    m_metadata.bitRate = m_formatCtx->bit_rate;
    m_metadata.preciseDurationMicroseconds = static_cast<long long>(effectiveDuration * 1000000.0);
    int bits = stream->codecpar->bits_per_raw_sample;
    if (bits <= 0) bits = av_get_bytes_per_sample(static_cast<AVSampleFormat>(stream->codecpar->format)) * 8;
    if (bits <= 0) bits = 16;
    m_metadata.sourceBitDepth = bits;
    m_useDoublePrecision = (m_metadata.sourceBitDepth > 32) ||
        (stream->codecpar->format == AV_SAMPLE_FMT_DBL) ||
        (stream->codecpar->format == AV_SAMPLE_FMT_DBLP);
    logFileDetails(m_formatCtx.get(), totalFileDuration);
    AVSampleFormat targetFormat = m_useDoublePrecision ? AV_SAMPLE_FMT_DBL : AV_SAMPLE_FMT_FLT;
    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, 1);
    SwrContext* swrCtxRaw = nullptr;
    swr_alloc_set_opts2(&swrCtxRaw, &out_layout, targetFormat, m_codecCtx->sample_rate,
        &m_codecCtx->ch_layout, m_codecCtx->sample_fmt, m_codecCtx->sample_rate, 0, nullptr);
    if (swrCtxRaw) {
        av_opt_set_int(swrCtxRaw, "linear_interp", 1, 0);
        av_opt_set_int(swrCtxRaw, "dither_scale", 0, 0);
    }
    m_swrCtx = FastTypes::SwrContextPtr(swrCtxRaw);
    if (!buildChannelMatrix(m_swrCtx.get(), m_codecCtx->ch_layout.nb_channels, targetChannelIdx)) return false;
    if (swr_init(m_swrCtx.get()) < 0) {
        emit logMessage(tr("        错误：重采样器初始化失败"));
        return false;
    }
    m_packet = FastTypes::AVPacketPtr(av_packet_alloc());
    m_frame = FastTypes::AVFramePtr(av_frame_alloc());
    emit logMessage(FastUtils::getCurrentTimestamp() + " " + tr("开始音频解码"));
    emit logMessage(tr("        [计算精度] %1 位浮点数").arg(m_useDoublePrecision ? 64 : 32));
    if (m_isRangeMode && m_startSec > 0.001) {
        seek(0.0);
    } else {
        m_nextExpectedTime = 0.0;
    }
    return true;
}

template <typename ContainerType>
void directResample(SwrContext* swr, AVFrame* frame, ContainerType& container, int offset, int count) {
    if (count <= 0) return;
    const uint8_t** input_data = (const uint8_t**)frame->data;
    const int max_channels = 8;
    const uint8_t* tmp_data[max_channels] = { nullptr };
    int channels = frame->ch_layout.nb_channels;
    if (channels > max_channels) channels = max_channels;
    int bytes_per_sample = av_get_bytes_per_sample((AVSampleFormat)frame->format);
    for(int i=0; i<channels; ++i) {
        if (frame->data[i]) {
            tmp_data[i] = frame->data[i] + offset * bytes_per_sample;
        }
    }
    size_t oldSize = container.size();
    int max_out = swr_get_out_samples(swr, count);
    if (max_out <= 0) return;
    container.resize(oldSize + max_out);
    uint8_t* out_data[1] = { reinterpret_cast<uint8_t*>(container.data() + oldSize) };
    int out_samples = swr_convert(swr, out_data, max_out, tmp_data, count);
    if (out_samples < max_out) {
        container.resize(oldSize + std::max(0, out_samples));
    }
}

template <typename ContainerType>
void flushResampler(SwrContext* swr, int sampleRate, ContainerType& container) {
    int delayed = swr_get_delay(swr, sampleRate);
    if (delayed > 0) {
        size_t oldSize = container.size();
        container.resize(oldSize + delayed);
        uint8_t* out_data[1] = { reinterpret_cast<uint8_t*>(container.data() + oldSize) };
        int out_samples = swr_convert(swr, out_data, delayed, nullptr, 0);
        if (out_samples < delayed) container.resize(oldSize + std::max(0, out_samples));
    }
}

FastTypes::FastPcmDataVariant FastAudioDecoder::readNextChunk(size_t minSamples) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    FastTypes::FastPcmData32 pcm32;
    FastTypes::FastPcmData64 pcm64;
    size_t alignmentElements = m_useDoublePrecision ? 8 : 16;
    size_t alignedSize = (minSamples + alignmentElements - 1) & ~(alignmentElements - 1);
    if (m_useDoublePrecision) pcm64.reserve(alignedSize);
    else pcm32.reserve(alignedSize);
    size_t samplesCollected = 0;
    bool decoderFullyDrained = false;
    while (samplesCollected < minSamples && !decoderFullyDrained) {
        if (!m_eofReached) {
            int ret = av_read_frame(m_formatCtx.get(), m_packet.get());
            if (ret < 0) {
                m_eofReached = true;
                avcodec_send_packet(m_codecCtx.get(), nullptr);
            } else {
                if (m_packet->stream_index == m_streamIndex) {
                    int sendRet = avcodec_send_packet(m_codecCtx.get(), m_packet.get());
                }
                av_packet_unref(m_packet.get());
            }
        }
        while (true) {
            int recvRet = avcodec_receive_frame(m_codecCtx.get(), m_frame.get());
            if (recvRet == AVERROR(EAGAIN)) {
                if (m_eofReached) {
                    decoderFullyDrained = true;
                }
                break;
            }
            else if (recvRet == AVERROR_EOF) {
                decoderFullyDrained = true;
                break;
            }
            else if (recvRet < 0) {
                decoderFullyDrained = true;
                break;
            }
            double frameStartTime = 0.0;
            if (m_frame->pts != AV_NOPTS_VALUE) {
                frameStartTime = m_frame->pts * m_timeBase;
            } else {
                frameStartTime = m_nextExpectedTime;
            }
            double frameDuration = (double)m_frame->nb_samples / m_codecCtx->sample_rate;
            double frameEndTime = frameStartTime + frameDuration;
            m_nextExpectedTime = frameEndTime;
            int skipSamples = 0;
            int processCount = m_frame->nb_samples;
            bool forceStopAfterThisFrame = false;
            if (m_isRangeMode) {
                if (frameEndTime <= m_startSec) {
                    av_frame_unref(m_frame.get());
                    continue;
                }
                if (m_endSec > 0.001 && frameStartTime >= m_endSec) {
                    if (!m_eofReached) {
                        m_eofReached = true;
                        avcodec_send_packet(m_codecCtx.get(), nullptr);
                    }
                    av_frame_unref(m_frame.get());
                    decoderFullyDrained = true; 
                    break;
                }
                if (frameStartTime < m_startSec) {
                    double diff = m_startSec - frameStartTime;
                    skipSamples = static_cast<int>(diff * m_codecCtx->sample_rate + 0.5);
                    if (skipSamples < 0) skipSamples = 0;
                    if (skipSamples >= processCount) skipSamples = processCount;
                }
                if (m_endSec > 0.001 && frameEndTime > m_endSec) {
                    double keepDuration = m_endSec - (frameStartTime + (double)skipSamples / m_codecCtx->sample_rate);
                    int keep = static_cast<int>(keepDuration * m_codecCtx->sample_rate + 0.5);
                    if (keep < 0) keep = 0;
                    if (keep < (processCount - skipSamples)) {
                        processCount = skipSamples + keep;
                    }
                    if (!m_eofReached) {
                        m_eofReached = true;
                        avcodec_send_packet(m_codecCtx.get(), nullptr);
                    }
                    forceStopAfterThisFrame = true;
                }
            }
            int validSamples = processCount - skipSamples;
            if (validSamples > 0) {
                size_t oldSize = m_useDoublePrecision ? pcm64.size() : pcm32.size();
                if (m_useDoublePrecision) {
                    directResample(m_swrCtx.get(), m_frame.get(), pcm64, skipSamples, validSamples);
                } else {
                    directResample(m_swrCtx.get(), m_frame.get(), pcm32, skipSamples, validSamples);
                }
                size_t newSize = m_useDoublePrecision ? pcm64.size() : pcm32.size();
                samplesCollected += (newSize - oldSize);
            }
            av_frame_unref(m_frame.get());
            if (forceStopAfterThisFrame) {
                decoderFullyDrained = true;
                break;
            }
        }
    }
    if (decoderFullyDrained) {
        if (m_useDoublePrecision) flushResampler(m_swrCtx.get(), m_codecCtx->sample_rate, pcm64);
        else flushResampler(m_swrCtx.get(), m_codecCtx->sample_rate, pcm32);
    }
    if (m_useDoublePrecision) return pcm64;
    return pcm32;
}

bool FastAudioDecoder::seek(double timestampSeconds) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_formatCtx || m_streamIndex < 0) return false;
    double targetAbsoluteSec = m_startSec + timestampSeconds;
    if (m_isRangeMode && m_endSec > 0.001 && targetAbsoluteSec > m_endSec) {
        targetAbsoluteSec = m_endSec;
    }
    int64_t targetTs = static_cast<int64_t>(targetAbsoluteSec * AV_TIME_BASE);
    if (m_formatCtx->streams[m_streamIndex]->time_base.den > 0) {
        targetTs = av_rescale_q(targetTs, { 1, AV_TIME_BASE }, m_formatCtx->streams[m_streamIndex]->time_base);
    }
    if (av_seek_frame(m_formatCtx.get(), m_streamIndex, targetTs, AVSEEK_FLAG_BACKWARD) < 0) {
        emit logMessage(tr("        警告：跳转失败"));
        return false;
    }
    avcodec_flush_buffers(m_codecCtx.get());
    m_eofReached = false;
    m_nextExpectedTime = targetAbsoluteSec;
    return true;
}