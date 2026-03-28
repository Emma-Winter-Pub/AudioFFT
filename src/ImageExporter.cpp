#include "ImageExporter.h"
#include "SpectrogramPainter.h"
#include "ImageEncoderFactory.h"
#include "IImageEncoder.h"

#include <memory>

ImageExporter::ExportResult ImageExporter::exportImage(
    const QImage &spectrogramImage,
    const QString &fileName,
    double audioDuration,
    bool showGrid,
    const QString &preciseDurationStr,
    int nativeSampleRate,
    int quality,
    const QString &outputFilePath,
    const QString &formatIdentifier,
    CurveType curveType,
    double minDb,
    double maxDb,
    PaletteType paletteType,
    bool drawComponents)
{
    SpectrogramPainter painter;
    QImage finalImage = painter.drawFinalImage(
        spectrogramImage,
        fileName,
        audioDuration,
        showGrid,
        preciseDurationStr,
        nativeSampleRate,
        curveType,
        minDb,
        maxDb,
        paletteType,
        drawComponents
    );
    if (finalImage.isNull()) {
        return {false, tr("图像绘制失败"), outputFilePath};
    }
    std::unique_ptr<IImageEncoder> encoder = ImageEncoderFactory::createEncoder(formatIdentifier);
    if (!encoder) {
         return {false, tr("找不到合适的图像编码器"), outputFilePath};
    }
    bool success = encoder->encodeAndSave(finalImage, outputFilePath, quality);
    if (success) {
        return {true, tr("图像保存成功"), outputFilePath};
    } else {
        return {false, tr("无法将图片保存到指定位置"), outputFilePath};
    }
}