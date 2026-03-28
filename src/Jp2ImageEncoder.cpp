#include "Jp2ImageEncoder.h"

#include <QImage>
#include <QBuffer>
#include <vector>
#include <stdexcept>
#include <openjpeg.h>
#include <cstring> 

namespace {
    void openjpeg_error_callback(const char *, void *) { }
    void openjpeg_warning_callback(const char *, void *) { }
    void openjpeg_info_callback(const char *, void *) { }
    OPJ_SIZE_T opj_mem_write(void* p_buffer, OPJ_SIZE_T p_nb_bytes, void* p_user_data) {
        QBuffer* buffer = static_cast<QBuffer*>(p_user_data);
        if (!buffer || !buffer->isOpen()) return static_cast<OPJ_SIZE_T>(-1);
        return static_cast<OPJ_SIZE_T>(buffer->write(static_cast<const char*>(p_buffer), p_nb_bytes));
    }
    OPJ_OFF_T opj_mem_skip(OPJ_OFF_T p_nb_bytes, void* p_user_data) {
        QBuffer* buffer = static_cast<QBuffer*>(p_user_data);
        if (!buffer || !buffer->isOpen()) return -1;
        qint64 newPos = buffer->pos() + p_nb_bytes;
        if (buffer->seek(newPos)) {
            return p_nb_bytes;
        }
        return -1;
    }
    OPJ_BOOL opj_mem_seek(OPJ_OFF_T p_nb_bytes, void* p_user_data) {
        QBuffer* buffer = static_cast<QBuffer*>(p_user_data);
        if (!buffer || !buffer->isOpen()) return OPJ_FALSE;
        if (buffer->seek(p_nb_bytes)) {
            return OPJ_TRUE;
        }
        return OPJ_FALSE;
    }
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
        component_params[i].dx = 1;
        component_params[i].dy = 1;
        component_params[i].w = width;
        component_params[i].h = height;
        component_params[i].prec = 8;
        component_params[i].bpp = 8;
        component_params[i].sgnd = 0;
        component_params[i].x0 = 0;
        component_params[i].y0 = 0;
    }
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
            opjImage->comps[2].data[pixel_index] = line[x * 3 + 2];
        }
    }
    opj_cparameters_t params;
    opj_set_default_encoder_parameters(&params);
    if (quality >= 100) {
        params.irreversible = 0;
    } else {
        params.irreversible = 1;
        params.tcp_numlayers = 1;
        params.tcp_distoratio[0] = static_cast<float>(quality);
        params.cp_fixed_quality = OPJ_TRUE;
    }
    opj_codec_t* codec = opj_create_compress(OPJ_CODEC_JP2);
    if (!codec) {
        opj_image_destroy(opjImage);
        return false;
    }
    opj_set_info_handler(codec, openjpeg_info_callback, nullptr);
    opj_set_warning_handler(codec, openjpeg_warning_callback, nullptr);
    opj_set_error_handler(codec, openjpeg_error_callback, nullptr);
    if (!opj_setup_encoder(codec, &params, opjImage)) {
        opj_destroy_codec(codec);
        opj_image_destroy(opjImage);
        return false;
    }
    opj_stream_t* stream = opj_stream_create_default_file_stream(filePath.toLocal8Bit().constData(), OPJ_FALSE);
    if (!stream) {
        opj_destroy_codec(codec);
        opj_image_destroy(opjImage);
        return false;
    }
    bool success = opj_start_compress(codec, opjImage, stream);
    if (success) success = opj_encode(codec, stream);
    if (success) success = opj_end_compress(codec, stream);
    opj_stream_destroy(stream);
    opj_destroy_codec(codec);
    opj_image_destroy(opjImage);
    return success;
}

QByteArray Jp2ImageEncoder::encodeToMemory(const QImage& image, int quality) const {
    QImage imageToSave = image.convertToFormat(QImage::Format_RGB888);
    if (imageToSave.isNull()) return QByteArray();
    const int width = imageToSave.width();
    const int height = imageToSave.height();
    const int numComponents = 3;
    opj_image_cmptparm_t component_params[numComponents];
    memset(component_params, 0, numComponents * sizeof(opj_image_cmptparm_t));
    for (int i = 0; i < numComponents; ++i) {
        component_params[i].dx = 1;
        component_params[i].dy = 1;
        component_params[i].w = width;
        component_params[i].h = height;
        component_params[i].prec = 8;
        component_params[i].bpp = 8;
        component_params[i].sgnd = 0;
        component_params[i].x0 = 0;
        component_params[i].y0 = 0;
    }
    opj_image_t* opjImage = opj_image_create(numComponents, component_params, OPJ_CLRSPC_SRGB);
    if (!opjImage) return QByteArray();
    opjImage->x0 = 0; opjImage->y0 = 0;
    opjImage->x1 = width; opjImage->y1 = height;
    for (int y = 0; y < height; ++y) {
        const uchar *line = imageToSave.scanLine(y);
        for (int x = 0; x < width; ++x) {
            int pixel_index = y * width + x;
            opjImage->comps[0].data[pixel_index] = line[x * 3];
            opjImage->comps[1].data[pixel_index] = line[x * 3 + 1];
            opjImage->comps[2].data[pixel_index] = line[x * 3 + 2];
        }
    }
    opj_cparameters_t params;
    opj_set_default_encoder_parameters(&params);
    if (quality >= 100) {
        params.irreversible = 0;
    } else {
        params.irreversible = 1;
        params.tcp_numlayers = 1;
        params.tcp_distoratio[0] = static_cast<float>(quality);
        params.cp_fixed_quality = OPJ_TRUE;
    }
    opj_codec_t* codec = opj_create_compress(OPJ_CODEC_JP2);
    if (!codec) { opj_image_destroy(opjImage); return QByteArray(); }
    opj_set_info_handler(codec, openjpeg_info_callback, nullptr);
    opj_set_warning_handler(codec, openjpeg_warning_callback, nullptr);
    opj_set_error_handler(codec, openjpeg_error_callback, nullptr);
    if (!opj_setup_encoder(codec, &params, opjImage)) {
        opj_destroy_codec(codec);
        opj_image_destroy(opjImage);
        return QByteArray();
    }
    QByteArray encodedData;
    QBuffer buffer(&encodedData);
    buffer.open(QIODevice::ReadWrite);
    opj_stream_t* stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_FALSE);
    if (!stream) {
        opj_destroy_codec(codec);
        opj_image_destroy(opjImage);
        return QByteArray();
    }
    opj_stream_set_user_data(stream, &buffer, nullptr);
    opj_stream_set_write_function(stream, opj_mem_write);
    opj_stream_set_skip_function(stream, opj_mem_skip);
    opj_stream_set_seek_function(stream, opj_mem_seek);
    opj_stream_set_user_data_length(stream, 0);
    bool success = opj_start_compress(codec, opjImage, stream);
    if (success) success = opj_encode(codec, stream);
    if (success) success = opj_end_compress(codec, stream);
    opj_stream_destroy(stream);
    opj_destroy_codec(codec);
    opj_image_destroy(opjImage);
    buffer.close();
    if (!success) return QByteArray();
    return encodedData;
}