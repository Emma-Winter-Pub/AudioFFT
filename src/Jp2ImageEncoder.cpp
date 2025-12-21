#include "Jp2ImageEncoder.h"

#include <QImage>
#include <vector>
#include <stdexcept>
#include <openjpeg.h>


namespace {
void openjpeg_error_callback(const char *, void *) { }
void openjpeg_warning_callback(const char *, void *) { }
void openjpeg_info_callback(const char *, void *) { }
}


bool Jp2ImageEncoder::encodeAndSave(const QImage& image, const QString& filePath, int quality) const {

    QImage imageToSave = image.convertToFormat(QImage::Format_RGB888);
    if (imageToSave.isNull()) {
        return false;
    }

    const int width = imageToSave.width();
    const int height = imageToSave.height();
    const int numComponents = 3;

    opj_image_cmptparm_t component_params[numComponents];
    memset(component_params, 0, numComponents * sizeof(opj_image_cmptparm_t));

    for (int i = 0; i < numComponents; ++i) {
        component_params[i].dx   = 1;
        component_params[i].dy   = 1;
        component_params[i].w    = width;
        component_params[i].h    = height;
        component_params[i].prec = 8;
        component_params[i].bpp  = 8;
        component_params[i].sgnd = 0;
        component_params[i].x0   = 0;
        component_params[i].y0   = 0;}

    opj_image_t* opjImage = opj_image_create(numComponents, component_params, OPJ_CLRSPC_SRGB);
    if (!opjImage) return false;

    opjImage->x0 = 0;
    opjImage->y0 = 0;
    opjImage->x1 = width;
    opjImage->y1 = height;

    for (int y = 0; y < height; ++y) {
        const uchar *line = imageToSave.scanLine(y);
        for (int x = 0; x < width; ++x) {
            int pixel_index = y * width + x;
            opjImage->comps[0].data[pixel_index] = line[x * 3];
            opjImage->comps[1].data[pixel_index] = line[x * 3 + 1];
            opjImage->comps[2].data[pixel_index] = line[x * 3 + 2];}}

    opj_cparameters_t params;
    opj_set_default_encoder_parameters(&params);
    if (quality >= 100) {
        params.irreversible = 0;}
    else {
        params.irreversible = 1;
        params.tcp_numlayers = 1;
        params.tcp_distoratio[0] = static_cast<float>(quality);
        params.cp_fixed_quality = OPJ_TRUE;}
    
    opj_codec_t* codec = opj_create_compress(OPJ_CODEC_JP2);
    if (!codec) {
        opj_image_destroy(opjImage);
        return false;}

    opj_set_info_handler(codec, openjpeg_info_callback, nullptr);
    opj_set_warning_handler(codec, openjpeg_warning_callback, nullptr);
    opj_set_error_handler(codec, openjpeg_error_callback, nullptr);

    if (!opj_setup_encoder(codec, &params, opjImage)) {
        opj_destroy_codec(codec);
        opj_image_destroy(opjImage);
        return false;}

    opj_stream_t* stream = opj_stream_create_default_file_stream(filePath.toLocal8Bit().constData(), OPJ_FALSE);
    if (!stream) {
        opj_destroy_codec(codec);
        opj_image_destroy(opjImage);
        return false;}

    bool success = opj_start_compress(codec, opjImage, stream);
    if (success) success = opj_encode(codec, stream);
    if (success) success = opj_end_compress(codec, stream);

    opj_stream_destroy(stream);
    opj_destroy_codec(codec);
    opj_image_destroy(opjImage);
    
    return success;
}