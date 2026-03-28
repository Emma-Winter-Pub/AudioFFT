#include "WebpImageEncoder.h"
#include "AppConfig.h"

#include <QFile>
#include <QImage>
#include <QByteArray>
#include <webp/encode.h>

bool WebpImageEncoder::encodeAndSave(const QImage& image, const QString& filePath, int quality) const{
    if (image.width() > AppConfig::MY_WEBP_MAX_DIMENSION || image.height() > AppConfig::MY_WEBP_MAX_DIMENSION) {
        return false;
    }
    QImage imageToSave = image.convertToFormat(QImage::Format_RGBA8888);
    if (imageToSave.isNull()) {
        return false;
    }
    const int width = imageToSave.width();
    const int height = imageToSave.height();
    const int stride = imageToSave.bytesPerLine();
    const uint8_t* pixels = imageToSave.constBits();
    uint8_t* output = nullptr;
    size_t output_size = 0;
    if (quality > 100) {
        output_size = WebPEncodeLosslessRGBA(pixels, width, height, stride, &output);
    } else {
        float quality_factor = static_cast<float>(quality);
        output_size = WebPEncodeRGBA(pixels, width, height, stride, quality_factor, &output);
    }
    if (output_size == 0) {
        return false;
    }
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        WebPFree(output);
        return false;
    }
    file.write(reinterpret_cast<const char*>(output), static_cast<qint64>(output_size));
    file.close();
    WebPFree(output);
    return true;
}

QByteArray WebpImageEncoder::encodeToMemory(const QImage& image, int quality) const{
    if (image.width() > AppConfig::MY_WEBP_MAX_DIMENSION || image.height() > AppConfig::MY_WEBP_MAX_DIMENSION) {
        return QByteArray();
    }
    QImage imageToSave = image.convertToFormat(QImage::Format_RGBA8888);
    if (imageToSave.isNull()) {
        return QByteArray();
    }
    const int width = imageToSave.width();
    const int height = imageToSave.height();
    const int stride = imageToSave.bytesPerLine();
    const uint8_t* pixels = imageToSave.constBits();
    uint8_t* output = nullptr;
    size_t output_size = 0;
    if (quality > 100) {
        output_size = WebPEncodeLosslessRGBA(pixels, width, height, stride, &output);
    } else {
        float quality_factor = static_cast<float>(quality);
        output_size = WebPEncodeRGBA(pixels, width, height, stride, quality_factor, &output);
    }
    if (output_size == 0) {
        return QByteArray();
    }
    QByteArray result(reinterpret_cast<const char*>(output), static_cast<int>(output_size));
    WebPFree(output);
    return result;
}