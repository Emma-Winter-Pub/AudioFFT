#pragma once

#include "MappingCurves.h"

#include <QString>
#include <cmath>

struct SpectrogramViewParams {
    int sampleRate = 0;
    double durationSec = 0.0;
    int imageHeight = 0;
    double pixelsPerSecond = 0.0;
    double zoomX = 1.0;
    double zoomY = 1.0;
    double offsetX = 0.0;
    double offsetY = 0.0;
    CurveType curveType = CurveType::XX;
    double minDb = -130.0;
    double maxDb = 0.0;
};

class SpectrogramCalculator{
public:
    SpectrogramCalculator() = default;
    void updateParams(const SpectrogramViewParams& params);
    QString calcFrequency(double mouseY, double viewRectTop) const;
    QString calcTime(double mouseX, double viewRectLeft) const;
    QString calcDb(int colorIndex) const;

private:
    double screenYToFreq(double mouseY, double viewRectTop) const;
    QString formatFreq(double freq, double delta) const;
    SpectrogramViewParams m_params;
};