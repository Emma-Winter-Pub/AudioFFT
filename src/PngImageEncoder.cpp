#include "PngImageEncoder.h"

#include <QImage>
#include <png.h>
#include <zlib.h>
#include <vector>
#include <cmath>
#include <stdexcept>


namespace PngEncoderHelpers {

    void user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
        std::string* err_str = static_cast<std::string*>(png_get_error_ptr(png_ptr));
        if (err_str) {
            *err_str = error_msg;}
        longjmp(png_jmpbuf(png_ptr), 1);}

    void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {
        (void)png_ptr;
        (void)warning_msg;}

} // namespace PngEncoderHelpers


bool PngImageEncoder::encodeAndSave(const QImage& image, const QString& filePath, int quality) const{

    QImage imageToSave = image.convertToFormat(QImage::Format_RGBA8888);
    if (imageToSave.isNull()) {
        return false;}

    FILE* fp = nullptr;
    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;
    std::string error_message;

#if defined(_MSC_VER)
    fp = _wfopen(filePath.toStdWString().c_str(), L"wb");
#else
    fp = fopen(filePath.toLocal8Bit().constData(), "wb");
#endif

    if (!fp) {
        return false;}

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, &error_message, PngEncoderHelpers::user_error_fn, PngEncoderHelpers::user_warning_fn);
    if (!png_ptr) {
        fclose(fp);
        return false;}

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, nullptr);
        fclose(fp);
        return false;}

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return false;}

    png_init_io(png_ptr, fp);

    int width = imageToSave.width();
    int height = imageToSave.height();
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
                 
 
    if (quality >= 0 && quality <= 9) {
        png_set_compression_level(png_ptr, quality);}

    png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_ALL_FILTERS);
    png_write_info(png_ptr, info_ptr);

    std::vector<png_bytep> row_pointers(height);
    for (int y = 0; y < height; ++y) {
        row_pointers[y] = (png_bytep)imageToSave.scanLine(y);}
    png_write_image(png_ptr, row_pointers.data());

    png_write_end(png_ptr, nullptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    return true;
}