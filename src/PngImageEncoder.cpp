#include "PngImageEncoder.h"

#include <QImage>
#include <QByteArray>
#include <png.h>
#include <zlib.h>
#include <vector>
#include <cmath>
#include <stdexcept>

namespace PngEncoderHelpers {
    void user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
        std::string* err_str = static_cast<std::string*>(png_get_error_ptr(png_ptr));
        if (err_str) *err_str = error_msg;
        longjmp(png_jmpbuf(png_ptr), 1);
    }
    void user_warning_fn(png_structp, png_const_charp) { }
}

bool PngImageEncoder::encodeAndSave(const QImage& image, const QString& filePath, int quality) const {
    QImage imageToSave = image;
    bool isIndexed = (image.format() == QImage::Format_Indexed8);
    if (!isIndexed && image.format() != QImage::Format_RGBA8888) {
        imageToSave = image.convertToFormat(QImage::Format_RGBA8888);
    }
    if (imageToSave.isNull()) return false;
    FILE* fp = nullptr;
#if defined(_MSC_VER)
    fp = _wfopen(filePath.toStdWString().c_str(), L"wb");
#else
    fp = fopen(filePath.toLocal8Bit().constData(), "wb");
#endif
    if (!fp) return false;
    std::string error_message;
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, &error_message, PngEncoderHelpers::user_error_fn, PngEncoderHelpers::user_warning_fn);
    if (!png_ptr) { fclose(fp); return false; }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) { png_destroy_write_struct(&png_ptr, nullptr); fclose(fp); return false; }
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return false;
    }
    png_init_io(png_ptr, fp);
    int width = imageToSave.width();
    int height = imageToSave.height();
    int color_type = isIndexed ? PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_RGBA;
    int bit_depth = 8;
    png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    std::vector<png_color> palette_storage;
    if (isIndexed) {
        const QList<QRgb>& colTable = imageToSave.colorTable();
        int num_colors = colTable.size();
        if (num_colors > 0 && num_colors <= 256) {
            palette_storage.resize(num_colors);
            for (int i = 0; i < num_colors; ++i) {
                palette_storage[i].red   = static_cast<png_byte>(qRed(colTable[i]));
                palette_storage[i].green = static_cast<png_byte>(qGreen(colTable[i]));
                palette_storage[i].blue  = static_cast<png_byte>(qBlue(colTable[i]));
            }
            png_set_PLTE(png_ptr, info_ptr, palette_storage.data(), num_colors);
        }
    }
    if (quality >= 0 && quality <= 9) {
        png_set_compression_level(png_ptr, quality);
    }
    int filters = isIndexed ? PNG_FILTER_NONE : PNG_ALL_FILTERS;
    png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, filters);
    png_write_info(png_ptr, info_ptr);
    std::vector<png_bytep> row_pointers(height);
    for (int y = 0; y < height; ++y) {
        row_pointers[y] = (png_bytep)imageToSave.constScanLine(y);
    }
    png_write_image(png_ptr, row_pointers.data());
    png_write_end(png_ptr, nullptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return true;
}

static void png_write_to_bytearray(png_structp png_ptr, png_bytep data, png_size_t length) {
    QByteArray* buffer = static_cast<QByteArray*>(png_get_io_ptr(png_ptr));
    if (buffer) {
        buffer->append(reinterpret_cast<const char*>(data), static_cast<int>(length));
    }
}

QByteArray PngImageEncoder::encodeToMemory(const QImage& image, int quality) const {
    QImage imageToSave = image;
    bool isIndexed = (image.format() == QImage::Format_Indexed8);
    if (!isIndexed && image.format() != QImage::Format_RGBA8888) {
        imageToSave = image.convertToFormat(QImage::Format_RGBA8888);
    }
    if (imageToSave.isNull()) return QByteArray();
    QByteArray outputBuffer;
    std::string error_message;
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, &error_message, PngEncoderHelpers::user_error_fn, PngEncoderHelpers::user_warning_fn);
    if (!png_ptr) return QByteArray();
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, nullptr);
        return QByteArray();
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return QByteArray();
    }
    png_set_write_fn(png_ptr, &outputBuffer, png_write_to_bytearray, nullptr);
    int width = imageToSave.width();
    int height = imageToSave.height();
    int color_type = isIndexed ? PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_RGBA;
    int bit_depth = 8;
    png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    std::vector<png_color> palette_storage;
    if (isIndexed) {
        const QList<QRgb>& colTable = imageToSave.colorTable();
        int num_colors = colTable.size();
        if (num_colors > 0 && num_colors <= 256) {
            palette_storage.resize(num_colors);
            for (int i = 0; i < num_colors; ++i) {
                palette_storage[i].red = static_cast<png_byte>(qRed(colTable[i]));
                palette_storage[i].green = static_cast<png_byte>(qGreen(colTable[i]));
                palette_storage[i].blue = static_cast<png_byte>(qBlue(colTable[i]));
            }
            png_set_PLTE(png_ptr, info_ptr, palette_storage.data(), num_colors);
        }
    }
    if (quality >= 0 && quality <= 9) {
        png_set_compression_level(png_ptr, quality);
    }
    int filters = isIndexed ? PNG_FILTER_NONE : PNG_ALL_FILTERS;
    png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, filters);
    png_write_info(png_ptr, info_ptr);
    std::vector<png_bytep> row_pointers(height);
    for (int y = 0; y < height; ++y) {
        row_pointers[y] = (png_bytep)imageToSave.constScanLine(y);
    }
    png_write_image(png_ptr, row_pointers.data());
    png_write_end(png_ptr, nullptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return outputBuffer;
}