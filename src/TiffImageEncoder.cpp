#include "TiffImageEncoder.h"

#include <QImage>
#include <QBuffer>
#include <tiffio.h>

namespace TiffMem {
    tsize_t readProc(thandle_t clientData, tdata_t buf, tsize_t size) {
        QBuffer* buffer = reinterpret_cast<QBuffer*>(clientData);
        return buffer->read((char*)buf, size);
    }
    tsize_t writeProc(thandle_t clientData, tdata_t buf, tsize_t size) {
        QBuffer* buffer = reinterpret_cast<QBuffer*>(clientData);
        return buffer->write((char*)buf, size);
    }
    toff_t seekProc(thandle_t clientData, toff_t off, int whence) {
        QBuffer* buffer = reinterpret_cast<QBuffer*>(clientData);
        if (whence == SEEK_SET) buffer->seek(off);
        else if (whence == SEEK_CUR) buffer->seek(buffer->pos() + off);
        else if (whence == SEEK_END) buffer->seek(buffer->size() + off);
        return buffer->pos();
    }
    int closeProc(thandle_t) { return 0; }
    toff_t sizeProc(thandle_t clientData) {
        QBuffer* buffer = reinterpret_cast<QBuffer*>(clientData);
        return buffer->size();
    }
    int mapProc(thandle_t, tdata_t*, toff_t*) { return 0; }
    void unmapProc(thandle_t, tdata_t, toff_t) {}
}

bool TiffImageEncoder::encodeAndSave(const QImage& image, const QString& filePath, int quality) const{
    (void)quality;
    QImage imageToSave = image.convertToFormat(QImage::Format_RGBA8888);
    if (imageToSave.isNull()) {
        return false;
    }
    TIFF* tif = TIFFOpen(filePath.toLocal8Bit().constData(), "w");
    if (!tif) {
        return false;
    }
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
            return false;
        }
    }
    TIFFClose(tif);
    return true;
}

QByteArray TiffImageEncoder::encodeToMemory(const QImage& image, int quality) const {
    (void)quality;
    QImage imageToSave = image.convertToFormat(QImage::Format_RGBA8888);
    if (imageToSave.isNull()) {
        return QByteArray();
    }
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadWrite);
    TIFF* tif = TIFFClientOpen("Mem", "w", (thandle_t)&buffer,
                               TiffMem::readProc, TiffMem::writeProc,
                               TiffMem::seekProc, TiffMem::closeProc,
                               TiffMem::sizeProc, TiffMem::mapProc, TiffMem::unmapProc);
    if (!tif) {
        return QByteArray();
    }
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
            return QByteArray();
        }
    }
    TIFFClose(tif);
    return bytes;
}