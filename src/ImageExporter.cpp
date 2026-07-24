#include "ImageExporter.h"
#include "FullLoadSpectrogramPainter.h"
#include "ImageEncoderFactory.h"
#include "XImageEncoder.h"

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
    const QString& paletteId,
    bool paletteInverted,
    bool paletteNegative,
    bool drawComponents)
{
    FullLoadSpectrogramPainter painter;
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
        paletteId,
        paletteInverted,
        paletteNegative,
        drawComponents
    );
    if (finalImage.isNull()) {
        return {false, tr("图像绘制失败"), outputFilePath};
    }
    std::unique_ptr<XImageEncoder> encoder = ImageEncoderFactory::createEncoder(formatIdentifier);
    if (!encoder) {
         return {false, tr("找不到合适的图像编码器"), outputFilePath};
    }
    bool success = encoder->EncodeAndSave(finalImage, outputFilePath, quality);
    if (success) {
        return {true, tr("图像保存成功"), outputFilePath};
    } else {
        return {false, tr("无法将图片保存到指定位置"), outputFilePath};
    }
}