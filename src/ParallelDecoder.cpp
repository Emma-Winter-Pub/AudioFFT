#include "ParallelDecoder.h"
#include "Utils.h"
#include "FFmpegRAII.h"

#include <thread>
#include <mutex>
#include <map>
#include <atomic>
#include <vector>


namespace {

    constexpr int FRAMES_TO_DISCARD_ON_SEEK = 31; // "Frames to discard = 31" is an empirical value to minimize stitching misalignment.

    struct DecodingChunk {
        int id;
        int64_t start_timestamp;
        int64_t end_timestamp;};

    void decode_worker(const QString& filePath, const DecodingChunk& chunk,
                       std::map<int, std::vector<float>>& all_results, std::mutex& results_mutex,
                       std::atomic<int>& progress_counter, ParallelDecoder* decoder)
    {
        std::vector<float> pcm_data;
        
        AVFormatContext* format_ctx_raw = nullptr;
        if (avformat_open_input(&format_ctx_raw, filePath.toUtf8().constData(), nullptr, nullptr) != 0) { return; }
        ffmpeg::FormatContextPtr format_ctx(format_ctx_raw);

        if (avformat_find_stream_info(format_ctx.get(), nullptr) < 0) { return; }
        
        int stream_index = av_find_best_stream(format_ctx.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (stream_index < 0) { return; }

        AVStream* stream = format_ctx->streams[stream_index];
        const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) { return; }

        ffmpeg::CodecContextPtr codec_ctx = ffmpeg::make_codec_context(codec);
        if (!codec_ctx) { return; }
        avcodec_parameters_to_context(codec_ctx.get(), stream->codecpar);
        avcodec_open2(codec_ctx.get(), codec, nullptr);
        
        SwrContext* swr_ctx_raw = nullptr;
        AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_MONO;
        swr_alloc_set_opts2(&swr_ctx_raw, 
                            &out_ch_layout, AV_SAMPLE_FMT_FLT, codec_ctx->sample_rate,
                            &codec_ctx->ch_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate, 
                            0, nullptr);
        ffmpeg::SwrContextPtr swr_ctx(swr_ctx_raw); 
        if (!swr_ctx) { return; }
        swr_init(swr_ctx.get());
        
        if (chunk.id > 0) {
            av_seek_frame(format_ctx.get(), stream_index, chunk.start_timestamp, AVSEEK_FLAG_BACKWARD);}

        ffmpeg::PacketPtr packet = ffmpeg::make_packet();
        ffmpeg::FramePtr frame = ffmpeg::make_frame();
        if (!packet || !frame) { return; }

        bool finished = false;
        int frames_to_discard = (chunk.id > 0) ? FRAMES_TO_DISCARD_ON_SEEK : 0;
        unsigned int num_threads = std::thread::hardware_concurrency();
        while (!finished && av_read_frame(format_ctx.get(), packet.get()) >= 0) {
            if (packet->stream_index == stream_index) {
                if (packet->pts >= chunk.end_timestamp && chunk.id < (int)num_threads - 1) {
                    finished = true;}
                if (avcodec_send_packet(codec_ctx.get(), packet.get()) >= 0) {
                    while (avcodec_receive_frame(codec_ctx.get(), frame.get()) >= 0) {
                        if (frames_to_discard > 0) {
                            frames_to_discard--;}
                        else {
                            int out_nb_samples = frame->nb_samples;
                            std::vector<float> buffer(out_nb_samples);
                            uint8_t* out_data[1] = { reinterpret_cast<uint8_t*>(buffer.data()) };
                            int converted_samples = swr_convert(swr_ctx.get(), out_data, out_nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
                            if (converted_samples > 0) {
                                pcm_data.insert(pcm_data.end(), buffer.begin(), buffer.begin() + converted_samples);}}
                        av_frame_unref(frame.get());}}}
            av_packet_unref(packet.get());}

        avcodec_send_packet(codec_ctx.get(), nullptr);
        while (avcodec_receive_frame(codec_ctx.get(), frame.get()) >= 0) {
             int out_nb_samples = frame->nb_samples;
             std::vector<float> buffer(out_nb_samples);
             uint8_t* out_data[1] = { reinterpret_cast<uint8_t*>(buffer.data()) };
             int converted_samples = swr_convert(swr_ctx.get(), out_data, out_nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
             if (converted_samples > 0) {
                pcm_data.insert(pcm_data.end(), buffer.begin(), buffer.begin() + converted_samples);}
             av_frame_unref(frame.get());}
        
        {
            std::lock_guard<std::mutex> lock(results_mutex);
            all_results[chunk.id] = std::move(pcm_data);}

        progress_counter++;}

} // namespace


ParallelDecoder::ParallelDecoder(QObject *parent) : QObject(parent) {}


bool ParallelDecoder::canHandle(const AudioMetadata& metadata){
    if (metadata.isApe) {
        return true;}
    return false;
}

std::vector<float> ParallelDecoder::decode(AVFormatContext* formatContext, const AudioMetadata& metadata){
    if (metadata.isApe) {
        return decodeApe(formatContext, metadata);}
    return {};
}


std::vector<float> ParallelDecoder::decodeApe(AVFormatContext* formatContext, const AudioMetadata& metadata){
    emit logMessage(getCurrentTimestamp() + tr(" Start parallel decoding"));
    unsigned int num_threads = std::thread::hardware_concurrency();
    emit logMessage(QString(tr("                        Decoding with %1 threads")).arg(num_threads));
    
    int audio_stream_index = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_index < 0) {
        avformat_close_input(&formatContext);
        return {};}
    
    AVStream* audio_stream = formatContext->streams[audio_stream_index];
    int64_t total_duration_ts = audio_stream->duration;

    avformat_close_input(&formatContext);
    
    std::vector<DecodingChunk> chunks;
    int64_t ts_per_chunk = total_duration_ts / num_threads;
    for (unsigned int i = 0; i < num_threads; ++i) {
        int64_t start_ts = i * ts_per_chunk;
        int64_t end_ts = (i == num_threads - 1) ? total_duration_ts : (i + 1) * ts_per_chunk;
        chunks.push_back({ (int)i, start_ts, end_ts });}

    std::map<int, std::vector<float>> all_results;
    std::vector<std::thread> threads;
    std::mutex results_mutex;
    std::atomic<int> progress_counter(0);
    
    for (const auto& chunk : chunks) {
        threads.emplace_back(decode_worker, metadata.filePath, std::ref(chunk),
            std::ref(all_results), std::ref(results_mutex), std::ref(progress_counter), this);}
    
    for (auto& t : threads) { t.join(); }
    
    std::vector<float> final_pcm_data;
    size_t total_samples = 0;
    for (size_t i = 0; i < all_results.size(); ++i) { 
        total_samples += all_results.at(i).size(); }
    final_pcm_data.reserve(total_samples);
    for (size_t i = 0; i < all_results.size(); ++i) { 
        final_pcm_data.insert(final_pcm_data.end(), all_results.at(i).begin(), all_results.at(i).end()); }
    
    return final_pcm_data;
}