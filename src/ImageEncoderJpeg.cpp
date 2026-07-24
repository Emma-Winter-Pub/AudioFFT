#include "ImageEncoderJpeg.h"

#include <QImage>
#include <QByteArray>
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
jpeg_error_exit(j_common_ptr cinfo) {
    JpegErrorManager* myerr = (JpegErrorManager*)cinfo->err;
    longjmp(myerr->setjmp_buffer, 1);
}

static bool encode_jpeg_to_file_core(const unsigned char* pixels, int width, int height, int bytesPerLine, int quality, FILE* fp) {
    jpeg_compress_struct cinfo;
    JpegErrorManager jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpeg_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_compress(&cinfo);
        return false;
    }
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);
    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW row_pointer = (JSAMPROW)(pixels + cinfo.next_scanline * bytesPerLine);
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    return true;
}

static bool encode_jpeg_to_memory_core(const unsigned char* pixels, int width, int height, int bytesPerLine, int quality, unsigned char** out_buffer, unsigned long* out_size) {
    jpeg_compress_struct cinfo;
    JpegErrorManager jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpeg_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_compress(&cinfo);
        return false;
    }
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, out_buffer, out_size);
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);
    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW row_pointer = (JSAMPROW)(pixels + cinfo.next_scanline * bytesPerLine);
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    return true;
}

bool ImageEncoderJpeg::EncodeAndSave(const QImage& image, const QString& filePath, int quality) const {
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
    bool success = encode_jpeg_to_file_core(
        imageToSave.constBits(),
        imageToSave.width(),
        imageToSave.height(),
        imageToSave.bytesPerLine(),
        quality,
        fp
    );
    fclose(fp);
    return success;
}

QByteArray ImageEncoderJpeg::EncodeToMemory(const QImage& image, int quality) const {
    QImage imageToSave = image.convertToFormat(QImage::Format_RGB888);
    if (imageToSave.isNull()) {
        return QByteArray();
    }
    unsigned char* mem_buffer = nullptr;
    unsigned long mem_size = 0;
    bool success = encode_jpeg_to_memory_core(
        imageToSave.constBits(),
        imageToSave.width(),
        imageToSave.height(),
        imageToSave.bytesPerLine(),
        quality,
        &mem_buffer,
        &mem_size
    );
    QByteArray result;
    if (success && mem_buffer && mem_size > 0) {
        result = QByteArray(reinterpret_cast<const char*>(mem_buffer), static_cast<int>(mem_size));
    }
    if (mem_buffer) {
        free(mem_buffer);
    }
    return result;
}