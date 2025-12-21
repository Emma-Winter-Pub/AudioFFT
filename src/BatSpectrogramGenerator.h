#pragma once

#include "MappingCurves.h"

#include <QImage>
#include <vector>


class BatSpectrogramGenerator{

public:
    BatSpectrogramGenerator();
    ~BatSpectrogramGenerator();

    QImage generate(
        const std::vector<std::vector<double>>& spectrogramData,
        int targetHeight,
        CurveType curveType);

};