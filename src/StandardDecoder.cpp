#include "StandardDecoder.h"
#include "Utils.h"

#include <vector>
#include <algorithm>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/rational.h>
}


static int decode_and_resample_single_thread(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt, SwrContext* swr_ctx, std::vector<float>& pcm_data) {
    int ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) return ret;
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return 0;
        if (ret < 0) return ret;
        
        int out_nb_samples = frame->nb_samples;
        std::vector<float> buffer(out_nb_samples);
        uint8_t* out_data[1] = { reinterpret_cast<uint8_t*>(buffer.data()) };
        
        int converted_samples = swr_convert(swr_ctx, out_data, out_nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
        if (converted_samples > 0) {
            pcm_data.insert(pcm_data.end(), buffer.begin(), buffer.begin() + converted_samples);}}
    return 0;
}


StandardDecoder::StandardDecoder(QObject *parent) : QObject(parent) {}


std::vector<float> StandardDecoder::decode(AVFormatContext* formatContext, AudioMetadata& metadata){

    emit logMessage(getCurrentTimestamp() + tr(" Decoding started"));
    
    std::vector<float> pcmData;

    int audio_stream_index = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_index < 0) {
        emit logMessage(tr("Audio stream not found during decoding."));
        return {};}

    AVStream* audio_stream = formatContext->streams[audio_stream_index];
    AVRational time_base = audio_stream->time_base;
    int64_t last_pts_plus_duration = 0;

    AVCodecParameters* dec_codec_params = audio_stream->codecpar;
    const AVCodec* dec_codec = avcodec_find_decoder(dec_codec_params->codec_id);
    AVCodecContext* dec_codec_ctx = avcodec_alloc_context3(dec_codec);
    avcodec_parameters_to_context(dec_codec_ctx, dec_codec_params);
    avcodec_open2(dec_codec_ctx, dec_codec, nullptr);
    
    SwrContext* swr_ctx = nullptr;
    AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_MONO;
    swr_alloc_set_opts2(&swr_ctx, 
                        &out_ch_layout, AV_SAMPLE_FMT_FLT, metadata.sampleRate,
                        &dec_codec_ctx->ch_layout, dec_codec_ctx->sample_fmt, dec_codec_ctx->sample_rate, 
                        0, nullptr);
    swr_init(swr_ctx);

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == audio_stream_index) {
            if (packet->pts != AV_NOPTS_VALUE) {
                int64_t current_end_ts = packet->pts + packet->duration;
                last_pts_plus_duration = std::max(last_pts_plus_duration, current_end_ts);}
            decode_and_resample_single_thread(dec_codec_ctx, frame, packet, swr_ctx, pcmData);}
        av_packet_unref(packet);}
    decode_and_resample_single_thread(dec_codec_ctx, frame, nullptr, swr_ctx, pcmData);

    if (last_pts_plus_duration > 0) {
        metadata.preciseDurationMicroseconds = static_cast<long long>(last_pts_plus_duration * av_q2d(time_base) * 1000000.0);}
    else {
        metadata.preciseDurationMicroseconds = formatContext->duration;}

    swr_free(&swr_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&dec_codec_ctx);
    
    return pcmData;
}