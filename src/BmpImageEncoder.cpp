#include "BmpImageEncoder.h"

#include <QImage>
#include <QBuffer>

bool BmpImageEncoder::encodeAndSave(const QImage& image, const QString& filePath, int quality) const{
    (void)quality; 
    return image.save(filePath, "BMP");
}

QByteArray BmpImageEncoder::encodeToMemory(const QImage& image, int quality) const{
    (void)quality;
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    if (image.save(&buffer, "BMP")) {
        return bytes;
    }
    return QByteArray();
}