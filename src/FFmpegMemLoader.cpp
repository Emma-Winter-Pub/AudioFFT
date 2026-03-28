#include "FFmpegMemLoader.h"

#include <QDebug>

static int read_packet(void *opaque, uint8_t *buf, int buf_size) {
    MemReaderState* state = static_cast<MemReaderState*>(opaque);
    size_t left = state->size - state->pos;
    if (left == 0) return AVERROR_EOF;
    int copy_size = (left < (size_t)buf_size) ? (int)left : buf_size;
    memcpy(buf, state->ptr + state->pos, copy_size);
    state->pos += copy_size;
    return copy_size;
}

static int64_t seek_packet(void *opaque, int64_t offset, int whence) {
    MemReaderState* state = static_cast<MemReaderState*>(opaque);
    if (whence == AVSEEK_SIZE) {
        return (int64_t)state->size;
    }
    int64_t new_pos = 0;
    switch (whence) {
        case SEEK_SET: new_pos = offset; break;
        case SEEK_CUR: new_pos = state->pos + offset; break;
        case SEEK_END: new_pos = state->size + offset; break;
        default: return -1;
    }
    if (new_pos < 0 || new_pos > (int64_t)state->size) return -1;
    state->pos = (size_t)new_pos;
    return new_pos;
}

SharedFileBuffer FFmpegMemLoader::loadFileToMemory(const QString& filePath) {
    FILE* file = nullptr;
#if defined(_WIN32)
    file = _wfopen(filePath.toStdWString().c_str(), L"rb");
#else
    file = fopen(filePath.toUtf8().constData(), "rb");
#endif
    if (!file) {
        return nullptr;
    }
    fseek(file, 0, SEEK_END);
    long long fileSize = _ftelli64(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize <= 0) {fclose(file); return nullptr; }
    auto buffer = std::make_shared<std::vector<uint8_t>>();
    try {
        buffer->resize(static_cast<size_t>(fileSize));
        size_t bytesRead = fread(buffer->data(), 1, static_cast<size_t>(fileSize), file);
        if (bytesRead != static_cast<size_t>(fileSize)) {
            fclose(file);
            return nullptr;
        }
    } catch (...) {
        fclose(file);
        return nullptr;
    }    
    fclose(file);
    return buffer;
}

AVFormatContext* FFmpegMemLoader::createFormatContextFromMemory(SharedFileBuffer buffer, MemReaderState* outState) {
    if (!buffer || buffer->empty() || !outState) return nullptr;
    outState->ptr = buffer->data();
    outState->size = buffer->size();
    outState->pos = 0;
    const int avio_buffer_size = 32768; // 32KB
    unsigned char* avio_buffer = (unsigned char*)av_malloc(avio_buffer_size);
    if (!avio_buffer) return nullptr;
    AVIOContext* avio_ctx = avio_alloc_context(
        avio_buffer, avio_buffer_size,
        0, 
        outState,
        read_packet, 
        nullptr, 
        seek_packet
    );
    if (!avio_ctx) {
        av_free(avio_buffer);
        return nullptr;
    }
    AVFormatContext* fmt_ctx = avformat_alloc_context();
    if (!fmt_ctx) {
        avio_context_free(&avio_ctx);
        return nullptr;
    }
    fmt_ctx->pb = avio_ctx;
    if (avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr) != 0) {
        cleanup(fmt_ctx);
        return nullptr;
    }
    return fmt_ctx;
}

void FFmpegMemLoader::cleanup(AVFormatContext* fmtCtx) {
    if (!fmtCtx) return;
    AVIOContext* pb = fmtCtx->pb;
    avformat_close_input(&fmtCtx);
    if (pb) {
        if (pb->buffer) {
            av_freep(&pb->buffer);
        }
        av_freep(&pb);
    }
}