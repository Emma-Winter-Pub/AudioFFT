#include "TiffImageEncoder.h"

#include <QImage>
#include <tiffio.h>


bool TiffImageEncoder::encodeAndSave(const QImage& image, const QString& filePath, int quality) const {

    (void)quality;

    QImage imageToSave = image.convertToFormat(QImage::Format_RGBA8888);
    if (imageToSave.isNull()) {
        return false;}
    
    TIFF* tif = TIFFOpen(filePath.toLocal8Bit().constData(), "w");
    if (!tif) {
        return false;}

    const int width = imageToSave.width();
    const int height = imageToSave.height();

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 4);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    
    uint16 extra_samples = 1; 
    uint16 sample_info[] = { EXTRASAMPLE_UNASSALPHA }; 
    TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, extra_samples, sample_info);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);

    for (int y = 0; y < height; ++y) {
        void* scanLine = (void*)imageToSave.scanLine(y);
        if (TIFFWriteScanline(tif, scanLine, y, 0) < 0) {
            TIFFClose(tif);
            return false;}}

    TIFFClose(tif);

    return true;
}