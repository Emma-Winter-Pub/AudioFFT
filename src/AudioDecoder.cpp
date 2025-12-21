#include "AudioDecoder.h"
#include "ParallelDecoder.h" 
#include "StandardDecoder.h" 
#include "FFmpegRAII.h"
#include "Utils.h"

#include <vector>
#include <sstream>
#include <QFileInfo>
#include <QDateTime>


static QString formatDuration(double total_seconds) {

    int hours   = static_cast<int>(total_seconds) / 3600;
    int minutes = static_cast<int>(total_seconds) / 60;
    int seconds = static_cast<int>(total_seconds) % 60;

    return QString("%1:%2:%3")
        .arg(hours)
        .arg(minutes)
        .arg(seconds);
}


AudioDecoder::AudioDecoder(QObject *parent) : QObject(parent) {}


AudioDecoder::~AudioDecoder() {}


bool AudioDecoder::decode(const QString& filePath){

    emit logMessage(getCurrentTimestamp() + tr(" Started parsing file"));

    AVFormatContext* formatContextRaw = nullptr;
    if (avformat_open_input(&formatContextRaw, filePath.toUtf8().constData(), nullptr, nullptr) != 0) {
        emit logMessage(tr("Cannot open file"));
        return false;}

    if (avformat_find_stream_info(formatContextRaw, nullptr) < 0) {
        avformat_close_input(&formatContextRaw);
        emit logMessage(tr("Audio stream not found in file"));
        return false;}

    m_metadata.filePath = filePath;

    if (!extractMetadata(formatContextRaw)) {
        avformat_close_input(&formatContextRaw);
        return false;}

    logFileDetails(formatContextRaw);

    if (!decodeAudio(formatContextRaw)) {return false;}

    emit logMessage(getCurrentTimestamp() + tr(" Decoding finished"));
    return true;
}


bool AudioDecoder::extractMetadata(AVFormatContext* formatContext){

    m_metadata.fileSize    = QFileInfo(m_metadata.filePath).size();
    m_metadata.durationSec = formatContext->duration / (double)AV_TIME_BASE;
    m_metadata.formatName  = formatContext->iformat->long_name;
    m_metadata.bitRate     = formatContext->bit_rate;

    int audio_stream_index = av_find_best_stream(formatContext,
                                                 AVMEDIA_TYPE_AUDIO,
                                                 -1,
                                                 -1,
                                                 nullptr,
                                                 0);

    if (audio_stream_index < 0) {
        emit logMessage(tr("Audio stream not found in file"));
        return false;}

    AVCodecParameters* codec_params = formatContext->streams[audio_stream_index]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);

    if (!codec) {
        emit logMessage(tr("Decoder not found"));
        return false;}

    m_metadata.codecName  = codec->long_name;
    m_metadata.sampleRate = codec_params->sample_rate;

    { ffmpeg::CodecContextPtr codec_ctx_info = ffmpeg::make_codec_context(codec);
        if (!codec_ctx_info) {
             emit logMessage(tr("Failed to allocate decoder context"));
             return false;}
        avcodec_parameters_to_context(codec_ctx_info.get(), codec_params);
        m_metadata.channelLayout = getChannelLayoutString(&codec_ctx_info->ch_layout); }

    m_metadata.isApe = (strcmp(formatContext->iformat->name, "ape") == 0);

    return true;
}


void AudioDecoder::logFileDetails(AVFormatContext* formatContext){

    emit logMessage(tr("File Information:"));
    emit logMessage(tr("    [Path]: ") + m_metadata.filePath);
    emit logMessage(tr("    [Size]: ") + formatSize(m_metadata.fileSize));
    emit logMessage(tr("    [Format]:%1").arg(m_metadata.formatName));
    emit logMessage(tr("Metadata:"));
    
    const AVDictionaryEntry* tag = nullptr;

    auto log_tag = [this, &tag](const char* key, const QString& display_name, AVFormatContext* ctx) {
        tag = av_dict_get(ctx->metadata, key, nullptr, AV_DICT_IGNORE_SUFFIX);
        if (tag && strlen(tag->value) > 0) emit logMessage(tr("    [%1]:%2")
            .arg(display_name)
            .arg(tag->value));};

    log_tag("title", tr("Title"),  formatContext); 
    log_tag("artist", tr("Artist"), formatContext); 
    log_tag("album", tr("Album"),  formatContext); 
    log_tag("track", tr("Track"),  formatContext); 
    log_tag("date", tr("Date"),   formatContext);
    
    emit logMessage(tr("Audio Information:"));
    emit logMessage(tr("    [Duration]: ") + formatDuration(m_metadata.durationSec));
    emit logMessage(tr("    [Codec]: %1").arg(m_metadata.codecName));
    emit logMessage(tr("    [Channel Layout]: ") + m_metadata.channelLayout);
    emit logMessage(tr("    [Bitrate]:%1 kbps").arg(static_cast<qint64>(m_metadata.bitRate / 1000)));
    emit logMessage(tr("    [Sample Rate]:%1Hz").arg(m_metadata.sampleRate));
}


QString AudioDecoder::getChannelLayoutString(const AVChannelLayout* ch_layout){
    char buf[128];
    av_channel_layout_describe(ch_layout, buf, sizeof(buf));
    return QString::fromUtf8(buf);
}


bool AudioDecoder::decodeAudio(AVFormatContext* formatContext){

    if (ParallelDecoder::canHandle(m_metadata)) {
        ParallelDecoder parallelDecoder;
        connect(&parallelDecoder, &ParallelDecoder::logMessage, this, &AudioDecoder::logMessage);
        m_metadata.preciseDurationMicroseconds = formatContext->duration;
        m_pcmData = parallelDecoder.decode(formatContext, m_metadata);} 
    else {
        StandardDecoder standardDecoder;
        connect(&standardDecoder, &StandardDecoder::logMessage, this, &AudioDecoder::logMessage);
        m_pcmData = standardDecoder.decode(formatContext, m_metadata);
        avformat_close_input(&formatContext);}

    if (m_pcmData.empty()) {
        emit logMessage(tr("No audio data after decoding"));
        return false;}

    return true;
}