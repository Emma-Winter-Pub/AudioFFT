#include "SpectrogramCalculator.h"
#include "Utils.h"

#include <QTime>
#include <QtMath>
#include <algorithm>

void SpectrogramCalculator::updateParams(const SpectrogramViewParams& params) {
    m_params = params;
}

double SpectrogramCalculator::screenYToFreq(double mouseY, double viewRectTop) const {
    if (m_params.imageHeight <= 1 || m_params.sampleRate <= 0) return 0.0;
    double imageY = (mouseY - viewRectTop - m_params.offsetY) / m_params.zoomY;
    if (imageY < 0.0) imageY = 0.0;
    if (imageY > m_params.imageHeight - 1.0) imageY = m_params.imageHeight - 1.0;
    double normalizedY = (m_params.imageHeight - 1.0 - imageY) / (m_params.imageHeight - 1.0);
    double maxFreq = m_params.sampleRate / 2.0;
    double linearRatio = MappingCurves::inverse(normalizedY, m_params.curveType, maxFreq);
    return linearRatio * maxFreq;
}

QString SpectrogramCalculator::formatFreq(double freq, double delta) const {
    if (delta < 0.1) {
        return QString("%1 Hz").arg(freq, 0, 'f', 2);
    } 
    else if (delta < 10.0) {
        return QString("%1 Hz").arg(freq, 0, 'f', 1);
    } 
    else if (delta < 1000.0) {
        return QString("%1 Hz").arg(std::round(freq));
    } 
    else {
        double kFreq = freq / 1000.0;
        return QString("%1k Hz").arg(kFreq, 0, 'f', 1);
    }
}

QString SpectrogramCalculator::calcFrequency(double mouseY, double viewRectTop) const {
    if (m_params.imageHeight <= 1) return "";
    double currentFreq = screenYToFreq(mouseY, viewRectTop);
    double nextFreq = screenYToFreq(mouseY + 1.0, viewRectTop);
    double delta = std::abs(currentFreq - nextFreq);
    return formatFreq(currentFreq, delta);
}

QString SpectrogramCalculator::calcTime(double mouseX, double viewRectLeft) const {
    if (m_params.durationSec <= 0.0 || m_params.pixelsPerSecond <= 0.0) return "00:00.000";
    double t = (mouseX - viewRectLeft - m_params.offsetX) / m_params.pixelsPerSecond;
    if (t < 0.0) t = 0.0;
    if (t > m_params.durationSec) t = m_params.durationSec;
    long long totalMs = static_cast<long long>(t * 1000.0);
    int ms = totalMs % 1000;
    long long totalSec = totalMs / 1000;
    int sec = totalSec % 60;
    int min = (totalSec / 60) % 60;
    int hour = totalSec / 3600;
    if (hour > 0) {
        return QString("%1:%2:%3.%4")
            .arg(hour)
            .arg(min, 2, 10, QChar('0'))
            .arg(sec, 2, 10, QChar('0'))
            .arg(ms, 3, 10, QChar('0'));
    } else {
        return QString("%1:%2.%3")
            .arg(min, 2, 10, QChar('0'))
            .arg(sec, 2, 10, QChar('0'))
            .arg(ms, 3, 10, QChar('0'));
    }
}

QString SpectrogramCalculator::calcDb(int colorIndex) const {
    if (colorIndex < 0 || colorIndex > 255) return "";
    double ratio = colorIndex / 255.0;
    double db = ratio * (m_params.maxDb - m_params.minDb) + m_params.minDb;
    int dbInt = static_cast<int>(std::round(db));
    return QString("%1 dBFS").arg(dbInt);
}