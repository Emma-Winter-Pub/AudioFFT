#include "ImageEncoderFactory.h"
#include "ImageEncoderPngLibpng.h"
#include "ImageEncoderPngQt.h"
#include "ImageEncoderJpeg.h"
#include "ImageEncoderBmpQt.h"
#include "ImageEncoderWebp.h"
#include "ImageEncoderTiff.h"
#include "ImageEncoderJpeg2000.h"
#include "ImageEncoderAvif.h"

#include <memory>

std::unique_ptr<XImageEncoder> ImageEncoderFactory::createEncoder(const QString& formatIdentifier) {
    if (formatIdentifier == "PNG")        { return std::make_unique<ImageEncoderPngLibpng>();}
    if (formatIdentifier == "QtPNG")      { return std::make_unique<ImageEncoderPngQt>();}
    if (formatIdentifier == "JPG")        { return std::make_unique<ImageEncoderJpeg>();}
    if (formatIdentifier == "BMP")        { return std::make_unique<ImageEncoderBmpQt>();}
    if (formatIdentifier == "WebP")       { return std::make_unique<ImageEncoderWebp>();}
    if (formatIdentifier == "TIFF")       { return std::make_unique<ImageEncoderTiff>();}
    if (formatIdentifier == "JPEG 2000")  { return std::make_unique<ImageEncoderJpeg2000>();}
    if (formatIdentifier == "AVIF")       { return std::make_unique<ImageEncoderAvif>();}
    return std::make_unique<ImageEncoderPngLibpng>();
}