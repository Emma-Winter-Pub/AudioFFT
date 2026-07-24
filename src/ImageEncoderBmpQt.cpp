#include "ImageEncoderBmpQt.h"

#include <QImage>
#include <QBuffer>

bool ImageEncoderBmpQt::EncodeAndSave(const QImage& image, const QString& filePath, int quality) const{
    (void)quality; 
    return image.save(filePath, "BMP");
}

QByteArray ImageEncoderBmpQt::EncodeToMemory(const QImage& image, int quality) const{
    (void)quality;
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    if (image.save(&buffer, "BMP")) {
        return bytes;
    }
    return QByteArray();
}