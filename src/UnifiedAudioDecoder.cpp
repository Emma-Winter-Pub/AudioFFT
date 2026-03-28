#include "UnifiedAudioDecoder.h"
#include "PipelineFactory.h"
#include "ApePipeline.h"
#include "DsdPipeline.h"
#include "FlacPipeline.h"
#include "Utils.h"
#include "StorageUtils.h"
#include "IParallelPipeline.h"

#include <QFileInfo>
#include <thread>
#include <QElapsedTimer>
#include <algorithm> 
#include <variant>

using namespace DecoderTypes;

struct PipelineRegistrar {
    PipelineRegistrar() {
        PipelineFactory::instance().registerPipeline("ape", []() { return std::make_unique<ApePipeline>(); });
        PipelineFactory::instance().registerPipeline("flac", []() { return std::make_unique<FlacPipeline>(); });
        PipelineFactory::instance().registerPipeline("alac", []() { return std::make_unique<FlacPipeline>(); });
        auto dsdCreator = []() { return std::make_unique<DsdPipeline>(); };
        PipelineFactory::instance().registerPipeline("dsd_lsbf", dsdCreator);
        PipelineFactory::instance().registerPipeline("dsd_msbf", dsdCreator);
        PipelineFactory::instance().registerPipeline("dsd_lsbf_planar", dsdCreator);
        PipelineFactory::instance().registerPipeline("dsd_msbf_planar", dsdCreator);
    }
};
static PipelineRegistrar g_registrar;

static QString formatDuration(double total_seconds) {
    long long totalSec = static_cast<long long>(total_seconds);
    int hours = totalSec / 3600;
    int minutes = (totalSec % 3600) / 60;
    int seconds = totalSec % 60;
    if (hours > 0) {
        return UnifiedAudioDecoder::tr("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return UnifiedAudioDecoder::tr("%1:%2")
            .arg(minutes)
            .arg(seconds, 2, 10, QChar('0'));
    }
}

UnifiedAudioDecoder::UnifiedAudioDecoder(QObject* parent) : QObject(parent) {}

QString UnifiedAudioDecoder::getChannelLayoutString(const AVChannelLayout* ch_layout){
    char buf[128];
    av_channel_layout_describe(ch_layout, buf, sizeof(buf));
    return QString::fromUtf8(buf);
}

void UnifiedAudioDecoder::logFileDetails(AVFormatContext* formatContext){
    emit logMessage(tr("    文件信息："));
    emit logMessage(tr("        [路径] %1").arg(m_metadata.filePath));
    emit logMessage(tr("        [大小] %1").arg(formatSize(QFileInfo(m_metadata.filePath).size())));
    emit logMessage(tr("        [时长] %1").arg(formatDuration(m_metadata.durationSec)));
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
        for (const auto& t : sheet.tracks) {
            QString startStr = QTime::fromMSecsSinceStartOfDay(static_cast<int>(t.startSeconds * 1000)).toString("HH:mm:ss");
            QString endStr;
            double endSec = t.endSeconds;
            if (endSec <= 0.001 && &t == &sheet.tracks.last()) {
                endSec = m_metadata.durationSec;
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
        if (tag && strlen(tag->value) > 0) {
            emit logMessage(tr("        [%1] %2").arg(display_name).arg(QString::fromUtf8(tag->value)));
        }
        };
    log_tag("title", tr("标题"), formatContext);
    log_tag("artist", tr("歌手"), formatContext);
    log_tag("album", tr("专辑"), formatContext);
    log_tag("track", tr("分轨"), formatContext);
    log_tag("date", tr("日期"), formatContext);
}

bool UnifiedAudioDecoder::buildChannelMatrix(SwrContext* swr, int inputChannels, int targetChannelIdx){
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

bool UnifiedAudioDecoder::decode(const QString& filePath, int targetTrackIdx, int targetChannelIdx, double startSec, double endSec, std::optional<CueSheet> cueSheet) {
    m_pcmData = PcmData32();
    m_metadata = DecoderTypes::AudioMetadata();
    m_metadata.tracks.clear();
    m_tempCueSheet = cueSheet;
    emit logMessage(getCurrentTimestamp() + " " + tr("探测文件"));
    AVFormatContext* formatCtxRaw = nullptr;
    if (avformat_open_input(&formatCtxRaw, filePath.toUtf8().constData(), nullptr, nullptr) != 0) {
        emit logMessage(tr("        错误：无法打开文件"));
        return false;
    }
    AVFormatContextPtr formatCtx(formatCtxRaw);
    if (avformat_find_stream_info(formatCtx.get(), nullptr) < 0) {
        emit logMessage(tr("        错误：无法获取流信息"));
        return false;
    }
    int bestStreamIdx = -1;
    for (unsigned int i = 0; i < formatCtx->nb_streams; ++i) {
        AVStream* st = formatCtx->streams[i];
        if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (bestStreamIdx < 0) bestStreamIdx = av_find_best_stream(formatCtx.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
            DecoderTypes::AudioTrack track;
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
            track.name = tr("音轨 %1: %2%3").arg(i).arg(codecStr.toUpper()).arg(lang.isEmpty() ? "" : " (" + lang + ")");
            m_metadata.tracks.append(track);
        }
    }
    if (m_metadata.tracks.isEmpty()) { emit logMessage(tr("        错误：未找到音频流")); return false; }
    int defaultIdx = (bestStreamIdx >= 0) ? bestStreamIdx : m_metadata.tracks[0].index;
    m_metadata.defaultTrackIndex = defaultIdx;
    int finalTrackIdx = targetTrackIdx < 0 ? defaultIdx : targetTrackIdx;
    m_metadata.currentTrackIndex = finalTrackIdx;
    AVStream* stream = formatCtx->streams[finalTrackIdx];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) { emit logMessage(tr("        错误：未找到解码器")); return false; }
    double totalDurationSec = 0.0;
    if (formatCtx->duration != AV_NOPTS_VALUE) {
        totalDurationSec = static_cast<double>(formatCtx->duration) / AV_TIME_BASE;
    }
    else if (stream->duration != AV_NOPTS_VALUE) {
        totalDurationSec = stream->duration * av_q2d(stream->time_base);
    }
    if (codec->id == AV_CODEC_ID_AAC || codec->id == AV_CODEC_ID_MP3) {
        int64_t hugeTimestamp = (stream->duration != AV_NOPTS_VALUE) ? stream->duration : (1LL << 60);
        if (av_seek_frame(formatCtx.get(), finalTrackIdx, hugeTimestamp, AVSEEK_FLAG_BACKWARD) >= 0) {
            DecoderTypes::AVPacketPtr tempPkt(av_packet_alloc());
            int64_t maxPts = 0;
            bool readAny = false;
            while (av_read_frame(formatCtx.get(), tempPkt.get()) >= 0) {
                if (tempPkt->stream_index == finalTrackIdx && tempPkt->pts != AV_NOPTS_VALUE) {
                    int64_t endPts = tempPkt->pts + tempPkt->duration;
                    if (endPts > maxPts) maxPts = endPts;
                    readAny = true;
                }
                av_packet_unref(tempPkt.get());
            }
            if (readAny && maxPts > 0) totalDurationSec = maxPts * av_q2d(stream->time_base);
        }
        av_seek_frame(formatCtx.get(), finalTrackIdx, 0, AVSEEK_FLAG_BACKWARD);
    }
    m_metadata.filePath = filePath;
    m_metadata.formatName = formatCtx->iformat->long_name ? formatCtx->iformat->long_name : formatCtx->iformat->name;
    m_metadata.codecName = codec->long_name ? codec->long_name : codec->name;
    m_metadata.sampleRate = stream->codecpar->sample_rate;
    m_metadata.channels = stream->codecpar->ch_layout.nb_channels;
    m_metadata.bitRate = formatCtx->bit_rate;
    int bits = stream->codecpar->bits_per_raw_sample;
    if (bits <= 0) bits = av_get_bytes_per_sample(static_cast<AVSampleFormat>(stream->codecpar->format)) * 8;
    if (bits <= 0) bits = 16;
    m_metadata.sourceBitDepth = bits;
    bool isRangeDecode = (startSec > 0.001 || endSec > 0.001);
    double effectiveEnd = endSec;
    if (effectiveEnd <= 0.001) effectiveEnd = totalDurationSec;
    else if (totalDurationSec > 0.001 && effectiveEnd > totalDurationSec) effectiveEnd = totalDurationSec;
    double effectiveDuration = (effectiveEnd > startSec) ? (effectiveEnd - startSec) : 0.0;
    m_metadata.durationSec = effectiveDuration;
    m_metadata.preciseDurationMicroseconds = static_cast<long long>(effectiveDuration * 1000000.0);
    logFileDetails(formatCtx.get());
    emit logMessage(getCurrentTimestamp() + " " + tr("开始音频解码"));
    bool useDouble = (m_metadata.sourceBitDepth > 32);
    emit logMessage(tr("        [计算精度] %1 位浮点数").arg(useDouble ? 64 : 32));
    if (isRangeDecode) {
        emit logMessage(tr("        解码范围：%1 秒 - %2 秒 (预估时长：%3 秒)")
            .arg(startSec, 0, 'f', 2).arg(effectiveEnd, 0, 'f', 2).arg(effectiveDuration, 0, 'f', 2));
    }
    std::string shortCodecName = codec->name;
    auto pipeline = PipelineFactory::instance().create(shortCodecName);
    bool success = false;
    if (pipeline) {
        auto logCb = [this](const QString& msg) { emit logMessage(msg); };
        qint64 fileSize = QFileInfo(filePath).size();
        bool isSmallEnough = (fileSize <= 1073741824LL);
        bool isShortEnough = (totalDurationSec < 0.001 || totalDurationSec < 7200.0);
        double pipeStart = isRangeDecode ? startSec : 0.0;
        double pipeEnd = isRangeDecode ? effectiveEnd : totalDurationSec;
        if (isSmallEnough && isShortEnough) {
            emit logMessage(tr("        [策略] 全量加载至内存"));
            success = pipeline->execute(filePath, finalTrackIdx, targetChannelIdx, m_metadata.sourceBitDepth, m_pcmData, m_metadata, logCb, ExecutionMode::LoadToRam, pipeStart, pipeEnd);
        }
        else {
            StorageType type = StorageUtils::detectStorageType(filePath);
            if (type == StorageType::SSD) {
                emit logMessage(tr("        [策略] 检测到高速设备 使用多线程"));
                success = pipeline->execute(filePath, finalTrackIdx, targetChannelIdx, m_metadata.sourceBitDepth, m_pcmData, m_metadata, logCb, ExecutionMode::DirectRead, pipeStart, pipeEnd);
            }
            else {
                emit logMessage(tr("        [策略] 检测到低速设备 使用单线程"));
                success = false;
            }
        }
    }
    if (!success) {
        if (pipeline) emit logMessage(tr("        多线程解码不可用或失败 回退到单线程"));
        else emit logMessage(tr("        [线程] 1 线程"));
        double stdStart = isRangeDecode ? startSec : 0.0;
        double stdEnd = isRangeDecode ? effectiveEnd : totalDurationSec;
        success = decodeStandard(filePath, finalTrackIdx, targetChannelIdx, stdStart, stdEnd);
    }
    bool hasData = std::visit([](auto&& arg) { return !arg.empty(); }, m_pcmData);
    if (success && hasData) {
        bool needCalibration = (m_metadata.durationSec <= 0.001) || !isRangeDecode;
        if (needCalibration) {
            size_t totalSamples = std::visit([](auto&& arg) { return arg.size(); }, m_pcmData);
            double pcmDuration = 0.0;
            if (m_metadata.sampleRate > 0 && totalSamples > 0) {
                pcmDuration = static_cast<double>(totalSamples) / m_metadata.sampleRate;
            }
            if (totalDurationSec > 0.001) {
                if (pcmDuration > totalDurationSec + 0.001) {
                    long long usec = static_cast<long long>(totalDurationSec * 1000000.0);
                    emit logMessage(tr("        [校准] 检测到填充数据 时长更新为 %1").arg(formatPreciseDuration(usec)));
                    m_metadata.durationSec = totalDurationSec;
                } else {
                    m_metadata.durationSec = pcmDuration;
                }
            } else {
                m_metadata.durationSec = pcmDuration;
            }
            m_metadata.preciseDurationMicroseconds = static_cast<long long>(m_metadata.durationSec * 1000000.0);
        }
        return true;
    }
    return false;
}

bool UnifiedAudioDecoder::decodeStandard(const QString& filePath, int trackIndex, int targetChannelIdx, double startSec, double endSec) {
    AVFormatContext* formatCtxRaw = nullptr;
    if (avformat_open_input(&formatCtxRaw, filePath.toUtf8().constData(), nullptr, nullptr) != 0) return false;
    AVFormatContextPtr formatCtx(formatCtxRaw);
    if (avformat_find_stream_info(formatCtx.get(), nullptr) < 0) return false;
    AVStream* stream = formatCtx->streams[trackIndex];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    AVCodecContextPtr codecCtx(avcodec_alloc_context3(codec));
    avcodec_parameters_to_context(codecCtx.get(), stream->codecpar);
    codecCtx->thread_count = 1;
    if (avcodec_open2(codecCtx.get(), codec, nullptr) < 0) return false;
    if (codecCtx->sample_rate > 0 && codecCtx->sample_rate != m_metadata.sampleRate) {
        emit logMessage(tr("        [修正] 采样率更新为 %1 Hz").arg(codecCtx->sample_rate));
        m_metadata.sampleRate = codecCtx->sample_rate;
    }
    bool useDoublePrecision = (m_metadata.sourceBitDepth > 32) ||
        (stream->codecpar->format == AV_SAMPLE_FMT_DBL) ||
        (stream->codecpar->format == AV_SAMPLE_FMT_DBLP);
    AVSampleFormat targetFormat = useDoublePrecision ? AV_SAMPLE_FMT_DBL : AV_SAMPLE_FMT_FLT;
    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, 1);
    SwrContext* swrCtxRaw = nullptr;
    swr_alloc_set_opts2(&swrCtxRaw, &out_layout, targetFormat, codecCtx->sample_rate,
        &codecCtx->ch_layout, codecCtx->sample_fmt, codecCtx->sample_rate, 0, nullptr);
    SwrContextPtr swrCtx(swrCtxRaw);
    if (!buildChannelMatrix(swrCtx.get(), codecCtx->ch_layout.nb_channels, targetChannelIdx)) return false;
    if (swr_init(swrCtx.get()) < 0) return false;
    double decodeDuration = (endSec > 0.001) ? (endSec - startSec) : m_metadata.durationSec;
    if (decodeDuration < 0) decodeDuration = 0;
    size_t estimatedSamples = 0;
    if (decodeDuration > 0 && codecCtx->sample_rate > 0) {
        estimatedSamples = static_cast<size_t>(decodeDuration * codecCtx->sample_rate * 1.1);
    }
    PcmData32 pcm32;
    PcmData64 pcm64;
    if (estimatedSamples > 0) {
        try {
            if (useDoublePrecision) pcm64.reserve(estimatedSamples);
            else pcm32.reserve(estimatedSamples);
        }
        catch (...) {}
    }
    if (startSec > 0.001) {
        int64_t targetTs = static_cast<int64_t>(startSec / av_q2d(stream->time_base));
        if (av_seek_frame(formatCtx.get(), trackIndex, targetTs, AVSEEK_FLAG_BACKWARD) < 0) {
            emit logMessage(tr("        [警告] 跳转失败 从开头读取"));
        }
        avcodec_flush_buffers(codecCtx.get());
    }
    AVPacketPtr pkt(av_packet_alloc());
    AVFramePtr frame(av_frame_alloc());
    auto appendData = [&](uint8_t* rawData, int count) {
        if (useDoublePrecision) {
            double* data = reinterpret_cast<double*>(rawData);
            pcm64.insert(pcm64.end(), data, data + count);
        }
        else {
            float* data = reinterpret_cast<float*>(rawData);
            pcm32.insert(pcm32.end(), data, data + count);
        }
    };
    while (av_read_frame(formatCtx.get(), pkt.get()) >= 0) {
        if (pkt->stream_index == trackIndex) {
            if (endSec > 0.001) {
                double currentPktTime = pkt->pts * av_q2d(stream->time_base);
                if (currentPktTime > endSec) {
                    av_packet_unref(pkt.get());
                    break;
                }
            }
            if (avcodec_send_packet(codecCtx.get(), pkt.get()) >= 0) {
                while (avcodec_receive_frame(codecCtx.get(), frame.get()) >= 0) {
                    int max_out = swr_get_out_samples(swrCtx.get(), frame->nb_samples);
                    int bytesPerSample = useDoublePrecision ? sizeof(double) : sizeof(float);
                    std::vector<uint8_t> rawBuffer(max_out * bytesPerSample);
                    uint8_t* out_data[1] = { rawBuffer.data() };

                    int out_samples = swr_convert(swrCtx.get(), out_data, max_out, (const uint8_t**)frame->data, frame->nb_samples);
                    if (out_samples > 0) {
                        appendData(rawBuffer.data(), out_samples);
                    }
                    av_frame_unref(frame.get());
                }
            }
        }
        av_packet_unref(pkt.get());
    }
    avcodec_send_packet(codecCtx.get(), nullptr);
    while (avcodec_receive_frame(codecCtx.get(), frame.get()) >= 0) {
        int max_out = swr_get_out_samples(swrCtx.get(), frame->nb_samples);
        int bytesPerSample = useDoublePrecision ? sizeof(double) : sizeof(float);
        std::vector<uint8_t> rawBuffer(max_out * bytesPerSample);
        uint8_t* out_data[1] = { rawBuffer.data() };
        int out_samples = swr_convert(swrCtx.get(), out_data, max_out, (const uint8_t**)frame->data, frame->nb_samples);
        if (out_samples > 0) appendData(rawBuffer.data(), out_samples);
        av_frame_unref(frame.get());
    }
    int delayed = swr_get_delay(swrCtx.get(), codecCtx->sample_rate);
    if (delayed > 0) {
        int max_out = swr_get_out_samples(swrCtx.get(), delayed);
        int bytesPerSample = useDoublePrecision ? sizeof(double) : sizeof(float);
        std::vector<uint8_t> rawBuffer(max_out * bytesPerSample);
        uint8_t* out_data[1] = { rawBuffer.data() };

        int out_samples = swr_convert(swrCtx.get(), out_data, max_out, nullptr, 0);
        if (out_samples > 0) {
            appendData(rawBuffer.data(), out_samples);
        }
    }
    if (useDoublePrecision) {
        if (!pcm64.empty()) m_pcmData = std::move(pcm64);
    }
    else {
        if (!pcm32.empty()) m_pcmData = std::move(pcm32);
    }
    return std::visit([](auto&& arg) { return !arg.empty(); }, m_pcmData);
}