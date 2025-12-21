#include "QtPngImageEncoder.h"

#include <QImage>

bool QtPngImageEncoder::encodeAndSave(const QImage& image, const QString& filePath, int quality) const {
    return image.save(filePath, "PNG", quality);
}