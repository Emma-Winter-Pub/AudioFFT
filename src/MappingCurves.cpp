#include "MappingCurves.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


namespace MappingCurves {

const double TAN_DIVISOR_9     = 1.0 / (2.0 * tan(9.0 * M_PI / 20.0));
const double ARCTAN_FACTOR_10  = 2.0 * tan(9.0 * M_PI / 20.0);
const double ARCTAN_DIVISOR_10 = 10.0 / (9.0 * M_PI);


double map(double linear_ratio, CurveType type){

    switch (type){

    // f(x)=x
        case CurveType::Function0:{
            return linear_ratio;}

    // f(x)=ln(99x+1)/ln100
        case CurveType::Function1:{
            return log1p(99.0 * linear_ratio) / log(100.0);}

    // f(x)=-ln(100-99x)/ln100+1
        case CurveType::Function2:{
            return 1.0 - log(100.0 - 99.0 * linear_ratio) / log(100.0);}

    // f(x)=sin(πx/2)
        case CurveType::Function3:{
            return sin(M_PI * linear_ratio / 2.0);}

    // f(x)=-cos(πx/2)+1
        case CurveType::Function4:{
            return 1.0 - cos(M_PI * linear_ratio / 2.0);}

    // f(x)=x²
        case CurveType::Function5:{
            return linear_ratio * linear_ratio;}

    // f(x)=-x²+2x
        case CurveType::Function6:{
            return 2.0 * linear_ratio - linear_ratio * linear_ratio;}

    // f(x)=x³
        case CurveType::Function7:{
            return linear_ratio * linear_ratio * linear_ratio;}

    // f(x)=(x-1)³+1
        case CurveType::Function8:{
            double val = linear_ratio - 1.0;
            return val * val * val + 1.0;}

    // f(x)=tan((9π/10)(x-1/2))/(2tan(9π/20))+1/2
        case CurveType::Function9:{
            return 0.5 + tan(0.9 * M_PI * (linear_ratio - 0.5)) * TAN_DIVISOR_9;}

    // f(x)=(10/(9π))arctan(2tan(9π/20)(x-1/2))+1/2
        case CurveType::Function10:{
            return 0.5 + ARCTAN_DIVISOR_10 * atan(ARCTAN_FACTOR_10 * (linear_ratio - 0.5));}

    // f(x)=4x³-6x²+3x
        case CurveType::Function11:{
            double x = linear_ratio;
            return (4.0 * x - 6.0) * x * x + 3.0 * x;}

    // f1(x)=4x³, x<1/2; f2(x)=4(x-1)³+1, x>=1/2
        case CurveType::Function12:{
            double x = linear_ratio;
            if (x < 0.5) {
                return 4.0 * x * x * x;}
            else {
                double val = x - 1.0;
                return 4.0 * val * val * val + 1.0;}}

        default:{
            return linear_ratio;}
    }
}

} // namespace MappingCurves