#include "BatAudioDecoder.h"
#include "BatFFmpegRAII.h"

#include <libavutil/opt.h>

namespace {

int decode_and_resample_packet(
    AVCodecContext* dec_ctx, 
    SwrContext* swr_ctx, 
    AVPacket* pkt, 
    BatFFmpeg::BatFramePtr& frame, 
    std::vector<float>& pcm_data)

{   
    int ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) return ret;
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame.get());
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return 0;
        if (ret < 0) return ret;
        const int out_nb_samples = frame->nb_samples;
        std::vector<float> buffer(out_nb_samples);
        uint8_t* out_data[1]  = { reinterpret_cast<uint8_t*>(buffer.data()) };
        int converted_samples = swr_convert(swr_ctx,
                                            out_data,
                                            out_nb_samples,
                                            (const uint8_t**)frame->data,
                                            frame->nb_samples);
        if (converted_samples > 0) {
            pcm_data.insert(pcm_data.end(),
                            buffer.begin(),
                            buffer.begin() + converted_samples);}
        av_frame_unref(frame.get());}

    return 0;
}

} // namespace


BatAudioDecoder::BatAudioDecoder() {}


BatAudioDecoder::~BatAudioDecoder() {}


std::optional<BatDecodedAudio> BatAudioDecoder::decode(const QString& filePath) {

    AVFormatContext* format_ctx_raw = nullptr;
    if (avformat_open_input(&format_ctx_raw,
                            filePath.toUtf8().constData(),
                            nullptr, nullptr) != 0) {
    return std::nullopt;}

    BatFFmpeg::BatFormatContextPtr formatContext(format_ctx_raw);

    if (avformat_find_stream_info(formatContext.get(), nullptr) < 0) {
        return std::nullopt;}

    int audio_stream_index = av_find_best_stream(formatContext.get(),
                                                 AVMEDIA_TYPE_AUDIO,
                                                 -1,
                                                 -1,
                                                 nullptr,
                                                 0);
    if (audio_stream_index < 0) {
        return std::nullopt;}

    AVStream* audio_stream = formatContext->streams[audio_stream_index];

    AVRational time_base   = audio_stream->time_base;
    int64_t last_pts_plus_duration = 0;

    const AVCodec* codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
    if (!codec) {
        return std::nullopt;}

    auto codec_ctx = BatFFmpeg::make_codec_context(codec);
    if (!codec_ctx || avcodec_parameters_to_context(codec_ctx.get(), audio_stream->codecpar) < 0) {
        return std::nullopt;}

    if (avcodec_open2(codec_ctx.get(), codec, nullptr) < 0) {
        return std::nullopt;}

    SwrContext* swr_ctx_raw = nullptr;
    AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_MONO;
    swr_alloc_set_opts2(&swr_ctx_raw,
                        &out_ch_layout,
                        AV_SAMPLE_FMT_FLT,
                        codec_ctx->sample_rate,
                        &codec_ctx->ch_layout,
                        codec_ctx->sample_fmt,
                        codec_ctx->sample_rate,
                        0,
                        nullptr);
    BatFFmpeg::BatSwrContextPtr swr_ctx(swr_ctx_raw);
    if (!swr_ctx || swr_init(swr_ctx.get()) < 0) {
        return std::nullopt;}

    auto packet = BatFFmpeg::make_packet();
    auto frame  = BatFFmpeg::make_frame();
    std::vector<float> pcmData;
    pcmData.reserve(static_cast<size_t>(formatContext->duration / (double)AV_TIME_BASE * codec_ctx->sample_rate));

    while (av_read_frame(formatContext.get(), packet.get()) >= 0) {
        if (packet->stream_index == audio_stream_index) {
            if (packet->pts != AV_NOPTS_VALUE) {
                int64_t current_end_ts = packet->pts + packet->duration;
                last_pts_plus_duration = std::max(last_pts_plus_duration, current_end_ts);}
            decode_and_resample_packet(codec_ctx.get(),
                                       swr_ctx.get(),
                                       packet.get(),
                                       frame,
                                       pcmData);}
        av_packet_unref(packet.get());}

    decode_and_resample_packet(codec_ctx.get(),
                               swr_ctx.get(),
                               nullptr,
                               frame,
                               pcmData);

    BatDecodedAudio result;
    result.pcmData    = std::move(pcmData);
    result.sampleRate = codec_ctx->sample_rate;
    
    if (last_pts_plus_duration > 0) {
        result.durationSec = static_cast<double>(last_pts_plus_duration) * av_q2d(time_base);}
    else {result.durationSec = (formatContext->duration == AV_NOPTS_VALUE) ? 0 : (formatContext->duration / (double)AV_TIME_BASE);}

    if (result.pcmData.empty()) {
        return std::nullopt;}

    return result;
}