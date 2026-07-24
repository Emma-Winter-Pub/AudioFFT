#pragma once

#include <vector>
#include <memory>
#include <QString>
#include <QFile>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/error.h>
}

using SharedFileBuffer = std::shared_ptr<std::vector<uint8_t>>;

struct MemReaderState {
    const uint8_t* ptr;
    size_t size;
    size_t pos;
};

class FFmpegMemLoader {
public:
    static SharedFileBuffer loadFileToMemory(const QString& filePath);
    static AVFormatContext* createFormatContextFromMemory(SharedFileBuffer buffer, MemReaderState* outState);
    static void cleanup(AVFormatContext* fmtCtx);
};