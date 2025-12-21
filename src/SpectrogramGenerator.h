#pragma once

#include "MappingCurves.h"

#include <QObject>
#include <QImage>
#include <vector>


class SpectrogramGenerator : public QObject {
    Q_OBJECT

public:
    explicit SpectrogramGenerator(QObject *parent = nullptr);

    QImage generate(const std::vector<std::vector<double>>& spectrogramData, int targetHeight, CurveType curveType);

signals:
    void logMessage(const QString& message);

};