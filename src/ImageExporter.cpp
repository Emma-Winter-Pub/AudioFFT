#include "ImageExporter.h"
#include "SpectrogramPainter.h"
#include "ImageEncoderFactory.h"
#include "IImageEncoder.h"

#include <memory>
#include <QObject>


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
    CurveType curveType) 
{
    SpectrogramPainter painter;
    QImage finalImage = painter.drawFinalImage(
        spectrogramImage,
        fileName,
        audioDuration,
        showGrid,
        preciseDurationStr,
        nativeSampleRate,
        curveType
    );

    if (finalImage.isNull()) {
        return {false, QObject::tr("Image rendering failed"), outputFilePath};}

    std::unique_ptr<IImageEncoder> encoder = ImageEncoderFactory::createEncoder(formatIdentifier);
    if (!encoder) {
         return {false, QObject::tr("Suitable image encoder not found"), outputFilePath};}
    
    bool success = encoder->encodeAndSave(finalImage, outputFilePath, quality);

    if (success) {
        return {true, QObject::tr("Image saved successfully"), outputFilePath};}
        else {
            return {false, QObject::tr("Cannot save image to the specified location"), outputFilePath};}

}