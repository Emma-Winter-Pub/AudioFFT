#include "AvifImageEncoder.h"
#include "AppConfig.h"

#include <QFile>
#include <QImage>
#include <QByteArray>
#include <avif/avif.h>
#include <algorithm>

bool AvifImageEncoder::encodeAndSave(const QImage& image, const QString& filePath, int quality) const {
    if (image.width() > AppConfig::MY_AVIF_MAX_DIMENSION || image.height() > AppConfig::MY_AVIF_MAX_DIMENSION) {
        return false;
    }
    QImage imageToSave = image.convertToFormat(QImage::Format_RGBA8888);
    if (imageToSave.isNull()) {
        return false;
    }
    avifImage* avifImage = avifImageCreate(imageToSave.width(), imageToSave.height(), 8, AVIF_PIXEL_FORMAT_YUV444);
    if (!avifImage) {
        return false;
    }
    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, avifImage);
    rgb.format = AVIF_RGB_FORMAT_RGBA;
    rgb.pixels = imageToSave.bits();
    rgb.rowBytes = imageToSave.bytesPerLine();
    if (avifImageRGBToYUV(avifImage, &rgb) != AVIF_RESULT_OK) {
        avifImageDestroy(avifImage);
        return false;
    }
    avifEncoder* encoder = avifEncoderCreate();
    if (!encoder) {
        avifImageDestroy(avifImage);
        return false;
    }
    encoder->speed = 6;
    if (quality >= 100) {
        encoder->minQuantizer = AVIF_QUANTIZER_LOSSLESS;
        encoder->maxQuantizer = AVIF_QUANTIZER_LOSSLESS;
        encoder->minQuantizerAlpha = AVIF_QUANTIZER_LOSSLESS;
        encoder->maxQuantizerAlpha = AVIF_QUANTIZER_LOSSLESS;
    } else {
        int qp = 63 - static_cast<int>((std::max(1, quality) / 99.0) * 63.0);
        encoder->minQuantizer = qp;
        encoder->maxQuantizer = qp;
        encoder->minQuantizerAlpha = qp;
        encoder->maxQuantizerAlpha = qp;
    }
    avifRWData output = { nullptr, 0 };
    avifResult result = avifEncoderWrite(encoder, avifImage, &output);
    avifEncoderDestroy(encoder);
    avifImageDestroy(avifImage);
    if (result != AVIF_RESULT_OK) {
        avifRWDataFree(&output);
        return false;
    }
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        avifRWDataFree(&output);
        return false;
    }
    file.write(reinterpret_cast<const char*>(output.data), static_cast<qint64>(output.size));
    file.close();
    avifRWDataFree(&output);
    return true;
}

QByteArray AvifImageEncoder::encodeToMemory(const QImage& image, int quality) const {
    if (image.width() > AppConfig::MY_AVIF_MAX_DIMENSION || image.height() > AppConfig::MY_AVIF_MAX_DIMENSION) {
        return QByteArray();
    }
    QImage imageToSave = image.convertToFormat(QImage::Format_RGBA8888);
    if (imageToSave.isNull()) {
        return QByteArray();
    }
    avifImage* avifImage = avifImageCreate(imageToSave.width(), imageToSave.height(), 8, AVIF_PIXEL_FORMAT_YUV444);
    if (!avifImage) return QByteArray();
    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, avifImage);
    rgb.format = AVIF_RGB_FORMAT_RGBA;
    rgb.pixels = imageToSave.bits();
    rgb.rowBytes = imageToSave.bytesPerLine();
    if (avifImageRGBToYUV(avifImage, &rgb) != AVIF_RESULT_OK) {
        avifImageDestroy(avifImage);
        return QByteArray();
    }
    avifEncoder* encoder = avifEncoderCreate();
    if (!encoder) {
        avifImageDestroy(avifImage);
        return QByteArray();
    }
    encoder->speed = 6;
    if (quality >= 100) {
        encoder->minQuantizer = AVIF_QUANTIZER_LOSSLESS;
        encoder->maxQuantizer = AVIF_QUANTIZER_LOSSLESS;
        encoder->minQuantizerAlpha = AVIF_QUANTIZER_LOSSLESS;
        encoder->maxQuantizerAlpha = AVIF_QUANTIZER_LOSSLESS;
    } else {
        int qp = 63 - static_cast<int>((std::max(1, quality) / 99.0) * 63.0);
        encoder->minQuantizer = qp;
        encoder->maxQuantizer = qp;
        encoder->minQuantizerAlpha = qp;
        encoder->maxQuantizerAlpha = qp;
    }
    avifRWData output = { nullptr, 0 };
    avifResult result = avifEncoderWrite(encoder, avifImage, &output);
    avifEncoderDestroy(encoder);
    avifImageDestroy(avifImage);
    if (result != AVIF_RESULT_OK) {
        avifRWDataFree(&output);
        return QByteArray();
    }
    QByteArray bytes(reinterpret_cast<const char*>(output.data), static_cast<int>(output.size));
    avifRWDataFree(&output);
    return bytes;
}