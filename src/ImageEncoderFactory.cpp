#include "ImageEncoderFactory.h"
#include "PngImageEncoder.h"
#include "QtPngImageEncoder.h"
#include "JpegImageEncoder.h"
#include "BmpImageEncoder.h"
#include "WebpImageEncoder.h"
#include "TiffImageEncoder.h"
#include "Jp2ImageEncoder.h"
#include "AvifImageEncoder.h"

#include <memory>


std::unique_ptr<IImageEncoder> ImageEncoderFactory::createEncoder(const QString& formatIdentifier) {

    if (formatIdentifier == "PNG") {
        return std::make_unique<PngImageEncoder>();}
    if (formatIdentifier == "QtPNG") {
        return std::make_unique<QtPngImageEncoder>();}
    if (formatIdentifier == "JPG") {
        return std::make_unique<JpegImageEncoder>();}
    if (formatIdentifier == "BMP") {
        return std::make_unique<BmpImageEncoder>();}
    if (formatIdentifier == "WebP") {
        return std::make_unique<WebpImageEncoder>();}
    if (formatIdentifier == "TIFF") {
        return std::make_unique<TiffImageEncoder>();}
    if (formatIdentifier == "JPEG 2000") {
        return std::make_unique<Jp2ImageEncoder>();}
    if (formatIdentifier == "AVIF") {
        return std::make_unique<AvifImageEncoder>();}

    return std::make_unique<PngImageEncoder>();

}