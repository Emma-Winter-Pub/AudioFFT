#include "ImageEncoderPngQt.h"

#include <QImage>
#include <QBuffer>

bool ImageEncoderPngQt::EncodeAndSave(const QImage& image, const QString& filePath, int quality) const {
    (void)quality;
    return image.save(filePath, "PNG");
}

QByteArray ImageEncoderPngQt::EncodeToMemory(const QImage& image, int quality) const {
    (void)quality;
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    if (image.save(&buffer, "PNG")) {
        return bytes;
    }
    return QByteArray();
}