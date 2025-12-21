#include "BmpImageEncoder.h"

#include <QImage>


bool BmpImageEncoder::encodeAndSave(const QImage& image, const QString& filePath, int quality) const {
    (void)quality; 
    return image.save(filePath, "BMP");
}