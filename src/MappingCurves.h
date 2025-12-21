#pragma once

enum class CurveType {
    Function0,
    Function1,
    Function2,
    Function3,
    Function4,
    Function5,
    Function6,
    Function7,
    Function8,
    Function9,
    Function10,
    Function11,
    Function12
};

namespace MappingCurves {

    double map(double linear_ratio, CurveType type);

} // namespace MappingCurves