#include "JpegImageEncoder.h"

#include <QImage>
#include <QByteArray>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
#include <jpeglib.h>
#include <jerror.h>
}

#include <csetjmp>

struct JpegErrorManager {
    jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

METHODDEF(void)
jpeg_error_exit(j_common_ptr cinfo)
{
    JpegErrorManager* myerr = (JpegErrorManager*)cinfo->err;
    longjmp(myerr->setjmp_buffer, 1);
}

bool JpegImageEncoder::encodeAndSave(const QImage& image, const QString& filePath, int quality) const{
    QImage imageToSave = image.convertToFormat(QImage::Format_RGB888);
    if (imageToSave.isNull()) {
        return false;
    }
    FILE* fp = nullptr;
#if defined(_MSC_VER)
    fp = _wfopen(filePath.toStdWString().c_str(), L"wb");
#else
    fp = fopen(filePath.toLocal8Bit().constData(), "wb");
#endif
    if (!fp) {
        return false;
    }
    jpeg_compress_struct cinfo;
    JpegErrorManager jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpeg_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_compress(&cinfo);
        fclose(fp);
        return false;
    }
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);
    cinfo.image_width = imageToSave.width();
    cinfo.image_height = imageToSave.height();
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);
    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW row_pointer = (JSAMPROW)imageToSave.scanLine(cinfo.next_scanline);
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(fp);
    return true;
}

QByteArray JpegImageEncoder::encodeToMemory(const QImage& image, int quality) const{
    QImage imageToSave = image.convertToFormat(QImage::Format_RGB888);
    if (imageToSave.isNull()) {
        return QByteArray();
    }
    jpeg_compress_struct cinfo;
    JpegErrorManager jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpeg_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_compress(&cinfo);
        return QByteArray();
    }
    jpeg_create_compress(&cinfo);
    unsigned char* mem_buffer = nullptr;
    unsigned long mem_size = 0;
    jpeg_mem_dest(&cinfo, &mem_buffer, &mem_size);
    cinfo.image_width = imageToSave.width();
    cinfo.image_height = imageToSave.height();
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);
    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW row_pointer = (JSAMPROW)imageToSave.scanLine(cinfo.next_scanline);
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    QByteArray result;
    if (mem_buffer && mem_size > 0) {
        result = QByteArray(reinterpret_cast<const char*>(mem_buffer), static_cast<int>(mem_size));
        free(mem_buffer);
    }
    return result;
}