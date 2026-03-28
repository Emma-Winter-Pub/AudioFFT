#include "MappingCurves.h"

#include <cmath>
#include <QtMath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const double pi = M_PI;

static const double C_A1_TAN_DENOM = 2.0 * std::tan(9.0 * pi / 20.0);
static const double C_A2_TAN_COEFF = 9.0 * pi / 10.0;
static const double C_A3_ATAN_COEFF = 10.0 / (9.0 * pi);
static const double C_A4_ATAN_INNER = 2.0 * std::tan(9.0 * pi / 20.0);

QList<CurveInfo> MappingCurves::getAllCurves(){
    static QList<CurveInfo> curves = {
        {CurveType::XX, "XX", "XX    f(x)=x", true},

        {CurveType::FM1, "FM1", "FM1  f(x)=1-(x-1)^2", false},
        {CurveType::FM2, "FM2", "FM2  f(x)=1+(x-1)^3", false},
        {CurveType::FM3, "FM3", "FM3  f(x)=1-(x-1)^4", false},
        {CurveType::FM4, "FM4", "FM4  f(x)=1+(x-1)^5", false},
        {CurveType::FM5, "FM5", "FM5  f(x)=1-(x-1)^6", false},
        {CurveType::FM6, "FM6", "FM6  f(x)=1+(x-1)^7", false},
        {CurveType::FM7, "FM7", "FM7  f(x)=1-(x-1)^8", false},
        {CurveType::FM8, "FM8", "FM8  f(x)=1+(x-1)^9", false},
        {CurveType::FM9, "FM9", "FM9  f(x)=1-(x-1)^10", true},

        {CurveType::M1, "M1", "M1  f(x)=x^2", false},
        {CurveType::M2, "M2", "M2  f(x)=x^3", false},
        {CurveType::M3, "M3", "M3  f(x)=x^4", false},
        {CurveType::M4, "M4", "M4  f(x)=x^5", false},
        {CurveType::M5, "M5", "M5  f(x)=x^6", false},
        {CurveType::M6, "M6", "M6  f(x)=x^7", false},
        {CurveType::M7, "M7", "M7  f(x)=x^8", false},
        {CurveType::M8, "M8", "M8  f(x)=x^9", false},
        {CurveType::M9, "M9", "M9  f(x)=x^10", true},

        {CurveType::D1, "D1", "D1    f(x)=lg(9x+1)", false},
        {CurveType::D2, "D2", "D2    f(x)=lg(99x+1)/2", false},
        {CurveType::D3, "D3", "D3    f(x)=lg(999x+1)/3", false},
        {CurveType::D4, "D4", "D4    f(x)=lg(9999x+1)/4", false},
        {CurveType::D5, "D5", "D5    f(x)=lg(99999x+1)/5", false},
        {CurveType::D6, "D6", "D6    f(x)=lg(999999x+1)/6", false},
        {CurveType::D7, "D7", "D7    f(x)=lg(9999999x+1)/7", false},
        {CurveType::D8, "D8", "D8    f(x)=lg(99999999x+1)/8", false},
        {CurveType::D9, "D9", "D9    f(x)=lg(999999999x+1)/9", false},
        {CurveType::D10, "D10", "D10  f(x)=lg(9999999999x+1)/10", true},

        {CurveType::FD1, "FD1", "FD1    f(x)=1-lg(10-9x)", false},
        {CurveType::FD2, "FD2", "FD2    f(x)=1-lg(100-99x)/2", false},
        {CurveType::FD3, "FD3", "FD3    f(x)=1-lg(1000-999x)/3", false},
        {CurveType::FD4, "FD4", "FD4    f(x)=1-lg(10000-9999x)/4", false},
        {CurveType::FD5, "FD5", "FD5    f(x)=1-lg(100000-99999x)/5", false},
        {CurveType::FD6, "FD6", "FD6    f(x)=1-lg(1000000-999999x)/6", false},
        {CurveType::FD7, "FD7", "FD7    f(x)=1-lg(10000000-9999999x)/7", false},
        {CurveType::FD8, "FD8", "FD8    f(x)=1-lg(100000000-99999999x)8", false},
        {CurveType::FD9, "FD9", "FD9    f(x)=1-lg(1000000000-999999999x)/9", false},
        {CurveType::FD10, "FD10", "FD10  f(x)=1-lg(10000000000-9999999999x)/10", true},

        {CurveType::Z1, "Z1", "Z1    f1(x)=9x/8,x<8/10; f2(x)=(x+1)/2,x≥8/10", false},
        {CurveType::Z2, "Z2", "Z2    f1(x)=9x/7,x<7/10; f2(x)=(x+2)/3,x≥8/10", false},
        {CurveType::Z3, "Z3", "Z3    f1(x)=9x/6,x<6/10; f2(x)=(x+3)/4,x≥8/10", false},
        {CurveType::Z4, "Z4", "Z4    f1(x)=9x/5,x<5/10; f2(x)=(x+4)/5,x≥8/10", false},
        {CurveType::Z5, "Z5", "Z5    f1(x)=9x/4,x<4/10; f2(x)=(x+5)/6,x≥8/10", false},
        {CurveType::Z6, "Z6", "Z6    f1(x)=9x/3,x<3/10; f2(x)=(x+6)/7,x≥8/10", false},
        {CurveType::Z7, "Z7", "Z7    f1(x)=9x/2,x<2/10; f2(x)=(x+7)/8,x≥8/10", false},
        {CurveType::Z8, "Z8", "Z8    f1(x)=9x/1,x<1/10; f2(x)=(x+8)/9,x≥8/10", true},

        {CurveType::FZ1, "FZ1", "FZ1  f1(x)=x/2,x<2/10; f2(x)=(9x-1)/8,x≥2/10", false},
        {CurveType::FZ2, "FZ2", "FZ2  f1(x)=x/3,x<3/10; f2(x)=(9x-2)/7,x≥3/10", false},
        {CurveType::FZ3, "FZ3", "FZ3  f1(x)=x/4,x<4/10; f2(x)=(9x-3)/6,x≥4/10", false},
        {CurveType::FZ4, "FZ4", "FZ4  f1(x)=x/5,x<5/10; f2(x)=(9x-4)/5,x≥5/10", false},
        {CurveType::FZ5, "FZ5", "FZ5  f1(x)=x/6,x<6/10; f2(x)=(9x-5)/4,x≥6/10", false},
        {CurveType::FZ6, "FZ6", "FZ6  f1(x)=x/7,x<7/10; f2(x)=(9x-6)/3,x≥7/10", false},
        {CurveType::FZ7, "FZ7", "FZ7  f1(x)=x/8,x<8/10; f2(x)=(9x-7)/2,x≥8/10", false},
        {CurveType::FZ8, "FZ8", "FZ8  f1(x)=x/9,x<9/10; f2(x)=(9x-8)/1,x≥9/10", true},

        {CurveType::Y1, "Y1", "Y1    f(x)=(2x-x^2)^(1/2)", false},
        {CurveType::FY1, "FY1", "FY1  f(x)=1-(1-x^2)^(1/2)", true},

        {CurveType::S1, "S1", "S1    f(x)=sin(πx/2)", false},
        {CurveType::FS1, "FS1", "FS1  f(x)=1-cos(πx/2)", true},

        {CurveType::S2, "S2", "S2    f(x)=tan((9π/10)(x-1/2))/(2tan(9π/20))+1/2", false},
        {CurveType::FS2, "FS2", "FS2  f(x)=(10/(9π))arctan(2tan(9π/20)(x-1/2))+1/2", true},

        {CurveType::H1, "H1", "H1    f(x)=4x^3-6x^2+3x", false},
        {CurveType::FH1, "FH1", "FH1  f1(x)=4x^3,x<1/2; f2(x)=4(x-1)^3+1,x≥1/2", true},

        {CurveType::SX1,  "SX1", "SX1  Mel(r)=ln(vr/700+1)/ln(v/700+1)", false},
        {CurveType::SX2, "SX2", "SX2  Bark(r)=(v+1960)r/(vr+1960)", false}
    };
    return curves;
}

QString MappingCurves::getName(CurveType type){
    const auto curves = getAllCurves();
    for (const auto& c : curves) {
        if (c.type == type) return c.displayText;
    }
    return "Unknown";
}

double MappingCurves::forward(double linear_ratio, CurveType type, double maxFreq){
    double x = linear_ratio;
    if (type != CurveType::SX1 && type != CurveType::SX2) {
        if (x < 0.0) x = 0.0;
        if (x > 1.0) x = 1.0;
    }
    switch (type) {

    case CurveType::XX: return x;

    case CurveType::FM1: return -std::pow(x - 1.0, 2.0) + 1.0;
    case CurveType::FM2: return std::pow(x - 1.0, 3.0) + 1.0;
    case CurveType::FM3: return -std::pow(x - 1.0, 4.0) + 1.0;
    case CurveType::FM4: return std::pow(x - 1.0, 5.0) + 1.0;
    case CurveType::FM5: return -std::pow(x - 1.0, 6.0) + 1.0;
    case CurveType::FM6: return std::pow(x - 1.0, 7.0) + 1.0;
    case CurveType::FM7: return -std::pow(x - 1.0, 8.0) + 1.0;
    case CurveType::FM8: return std::pow(x - 1.0, 9.0) + 1.0;
    case CurveType::FM9: return -std::pow(x - 1.0, 10.0) + 1.0;

    case CurveType::M1: return std::pow(x, 2.0);
    case CurveType::M2: return std::pow(x, 3.0);
    case CurveType::M3: return std::pow(x, 4.0);
    case CurveType::M4: return std::pow(x, 5.0);
    case CurveType::M5: return std::pow(x, 6.0);
    case CurveType::M6: return std::pow(x, 7.0);
    case CurveType::M7: return std::pow(x, 8.0);
    case CurveType::M8: return std::pow(x, 9.0);
    case CurveType::M9: return std::pow(x, 10.0);

    case CurveType::D1: return std::log((std::pow(10.0, 1.0) - 1.0) * x + 1.0) / std::log(std::pow(10.0, 1.0));
    case CurveType::D2: return std::log((std::pow(10.0, 2.0) - 1.0) * x + 1.0) / std::log(std::pow(10.0, 2.0));
    case CurveType::D3: return std::log((std::pow(10.0, 3.0) - 1.0) * x + 1.0) / std::log(std::pow(10.0, 3.0));
    case CurveType::D4: return std::log((std::pow(10.0, 4.0) - 1.0) * x + 1.0) / std::log(std::pow(10.0, 4.0));
    case CurveType::D5: return std::log((std::pow(10.0, 5.0) - 1.0) * x + 1.0) / std::log(std::pow(10.0, 5.0));
    case CurveType::D6: return std::log((std::pow(10.0, 6.0) - 1.0) * x + 1.0) / std::log(std::pow(10.0, 6.0));
    case CurveType::D7: return std::log((std::pow(10.0, 7.0) - 1.0) * x + 1.0) / std::log(std::pow(10.0, 7.0));
    case CurveType::D8: return std::log((std::pow(10.0, 8.0) - 1.0) * x + 1.0) / std::log(std::pow(10.0, 8.0));
    case CurveType::D9: return std::log((std::pow(10.0, 9.0) - 1.0) * x + 1.0) / std::log(std::pow(10.0, 9.0));
    case CurveType::D10: return std::log((std::pow(10.0, 10.0) - 1.0) * x + 1.0) / std::log(std::pow(10.0, 10.0));

    case CurveType::FD1: return -std::log(std::pow(10.0, 1.0) - (std::pow(10.0, 1.0) - 1.0) * x) / std::log(std::pow(10.0, 1.0)) + 1.0;
    case CurveType::FD2: return -std::log(std::pow(10.0, 2.0) - (std::pow(10.0, 2.0) - 1.0) * x) / std::log(std::pow(10.0, 2.0)) + 1.0;
    case CurveType::FD3: return -std::log(std::pow(10.0, 3.0) - (std::pow(10.0, 3.0) - 1.0) * x) / std::log(std::pow(10.0, 3.0)) + 1.0;
    case CurveType::FD4: return -std::log(std::pow(10.0, 4.0) - (std::pow(10.0, 4.0) - 1.0) * x) / std::log(std::pow(10.0, 4.0)) + 1.0;
    case CurveType::FD5: return -std::log(std::pow(10.0, 5.0) - (std::pow(10.0, 5.0) - 1.0) * x) / std::log(std::pow(10.0, 5.0)) + 1.0;
    case CurveType::FD6: return -std::log(std::pow(10.0, 6.0) - (std::pow(10.0, 6.0) - 1.0) * x) / std::log(std::pow(10.0, 6.0)) + 1.0;
    case CurveType::FD7: return -std::log(std::pow(10.0, 7.0) - (std::pow(10.0, 7.0) - 1.0) * x) / std::log(std::pow(10.0, 7.0)) + 1.0;
    case CurveType::FD8: return -std::log(std::pow(10.0, 8.0) - (std::pow(10.0, 8.0) - 1.0) * x) / std::log(std::pow(10.0, 8.0)) + 1.0;
    case CurveType::FD9: return -std::log(std::pow(10.0, 9.0) - (std::pow(10.0, 9.0) - 1.0) * x) / std::log(std::pow(10.0, 9.0)) + 1.0;
    case CurveType::FD10: return -std::log(std::pow(10.0, 10.0) - (std::pow(10.0, 10.0) - 1.0) * x) / std::log(std::pow(10.0, 10.0)) + 1.0;

    case CurveType::Z1: if (x < 0.8) return 9.0 * x / 8.0; else return (x + 1.0) / 2.0;
    case CurveType::Z2: if (x < 0.7) return 9.0 * x / 7.0; else return (x + 2.0) / 3.0;
    case CurveType::Z3: if (x < 0.6) return 9.0 * x / 6.0; else return (x + 3.0) / 4.0;
    case CurveType::Z4: if (x < 0.5) return 9.0 * x / 5.0; else return (x + 4.0) / 5.0;
    case CurveType::Z5: if (x < 0.4) return 9.0 * x / 4.0; else return (x + 5.0) / 6.0;
    case CurveType::Z6: if (x < 0.3) return 9.0 * x / 3.0; else return (x + 6.0) / 7.0;
    case CurveType::Z7: if (x < 0.2) return 9.0 * x / 2.0; else return (x + 7.0) / 8.0;
    case CurveType::Z8: if (x < 0.1) return 9.0 * x / 1.0; else return (x + 8.0) / 9.0;

    case CurveType::FZ1: if (x < 0.2) return x / 2.0; else return (9 * x - 1.0) / 8.0;
    case CurveType::FZ2: if (x < 0.3) return x / 3.0; else return (9 * x - 2.0) / 7.0;
    case CurveType::FZ3: if (x < 0.4) return x / 4.0; else return (9 * x - 3.0) / 6.0;
    case CurveType::FZ4: if (x < 0.5) return x / 5.0; else return (9 * x - 4.0) / 5.0;
    case CurveType::FZ5: if (x < 0.6) return x / 6.0; else return (9 * x - 5.0) / 4.0;
    case CurveType::FZ6: if (x < 0.7) return x / 7.0; else return (9 * x - 6.0) / 3.0;
    case CurveType::FZ7: if (x < 0.8) return x / 8.0; else return (9 * x - 7.0) / 2.0;
    case CurveType::FZ8: if (x < 0.9) return x / 9.0; else return (9 * x - 8.0) / 1.0;

    case CurveType::Y1: return std::pow(2.0 * x - x * x, 0.5);
    case CurveType::FY1: return 1.0 - std::pow(1.0 - x * x, 0.5);

    case CurveType::S1: return std::sin(pi * x / 2.0);
    case CurveType::FS1: return -std::cos(pi * x / 2.0) + 1.0;

    case CurveType::S2: return std::tan(C_A2_TAN_COEFF * (x - 0.5)) / C_A1_TAN_DENOM + 0.5;
    case CurveType::FS2: return C_A3_ATAN_COEFF * std::atan(C_A4_ATAN_INNER * (x - 0.5)) + 0.5;

    case CurveType::H1: return 4.0 * std::pow(x, 3.0) - 6.0 * std::pow(x, 2.0) + 3.0 * x;
    case CurveType::FH1: if (x < 0.5) return 4.0 * std::pow(x, 3.0); else return 4.0 * std::pow(x - 1.0, 3.0) + 1.0;

    case CurveType::SX1: if (maxFreq <= 0.0) return x; return std::log(maxFreq * x / 700.0 + 1.0) / std::log(maxFreq / 700.0 + 1.0);
    case CurveType::SX2: return ((maxFreq + 1960.0) * x) / (maxFreq * x + 1960.0);

    default: return x;
    }
}

double MappingCurves::inverse(double y_ratio, CurveType type, double maxFreq){
    double y = y_ratio;
    if (type != CurveType::SX1 && type != CurveType::SX2) {
        if (y < 0.0) y = 0.0;
        if (y > 1.0) y = 1.0;
    }
    switch (type) {

    case CurveType::XX: return y;

    case CurveType::FM1: return 1.0 - std::pow(1.0 - y, 1.0 / 2.0);
    case CurveType::FM2: return 1.0 - std::pow(1.0 - y, 1.0 / 3.0);
    case CurveType::FM3: return 1.0 - std::pow(1.0 - y, 1.0 / 4.0);
    case CurveType::FM4: return 1.0 - std::pow(1.0 - y, 1.0 / 5.0);
    case CurveType::FM5: return 1.0 - std::pow(1.0 - y, 1.0 / 6.0);
    case CurveType::FM6: return 1.0 - std::pow(1.0 - y, 1.0 / 7.0);
    case CurveType::FM7: return 1.0 - std::pow(1.0 - y, 1.0 / 8.0);
    case CurveType::FM8: return 1.0 - std::pow(1.0 - y, 1.0 / 9.0);
    case CurveType::FM9: return 1.0 - std::pow(1.0 - y, 1.0 / 10.0);

    case CurveType::M1: return std::pow(y, 1.0 / 2.0);
    case CurveType::M2: return std::pow(y, 1.0 / 3.0);
    case CurveType::M3: return std::pow(y, 1.0 / 4.0);
    case CurveType::M4: return std::pow(y, 1.0 / 5.0);
    case CurveType::M5: return std::pow(y, 1.0 / 6.0);
    case CurveType::M6: return std::pow(y, 1.0 / 7.0);
    case CurveType::M7: return std::pow(y, 1.0 / 8.0);
    case CurveType::M8: return std::pow(y, 1.0 / 9.0);
    case CurveType::M9: return std::pow(y, 1.0 / 10.0);

    case CurveType::D1: return (std::pow(10.0, 1.0 * y) - 1.0) / (std::pow(10.0, 1.0) - 1.0);
    case CurveType::D2: return (std::pow(10.0, 2.0 * y) - 1.0) / (std::pow(10.0, 2.0) - 1.0);
    case CurveType::D3: return (std::pow(10.0, 3.0 * y) - 1.0) / (std::pow(10.0, 3.0) - 1.0);
    case CurveType::D4: return (std::pow(10.0, 4.0 * y) - 1.0) / (std::pow(10.0, 4.0) - 1.0);
    case CurveType::D5: return (std::pow(10.0, 5.0 * y) - 1.0) / (std::pow(10.0, 5.0) - 1.0);
    case CurveType::D6: return (std::pow(10.0, 6.0 * y) - 1.0) / (std::pow(10.0, 6.0) - 1.0);
    case CurveType::D7: return (std::pow(10.0, 7.0 * y) - 1.0) / (std::pow(10.0, 7.0) - 1.0);
    case CurveType::D8: return (std::pow(10.0, 8.0 * y) - 1.0) / (std::pow(10.0, 8.0) - 1.0);
    case CurveType::D9: return (std::pow(10.0, 9.0 * y) - 1.0) / (std::pow(10.0, 9.0) - 1.0);
    case CurveType::D10: return (std::pow(10.0, 10.0 * y) - 1.0) / (std::pow(10.0, 10.0) - 1.0);

    case CurveType::FD1: return (std::pow(10.0, 1.0) - std::pow(10.0, 1.0 - 1.0 * y)) / (std::pow(10.0, 1.0) - 1.0);
    case CurveType::FD2: return (std::pow(10.0, 2.0) - std::pow(10.0, 2.0 - 2.0 * y)) / (std::pow(10.0, 2.0) - 1.0);
    case CurveType::FD3: return (std::pow(10.0, 3.0) - std::pow(10.0, 3.0 - 3.0 * y)) / (std::pow(10.0, 3.0) - 1.0);
    case CurveType::FD4: return (std::pow(10.0, 4.0) - std::pow(10.0, 4.0 - 4.0 * y)) / (std::pow(10.0, 4.0) - 1.0);
    case CurveType::FD5: return (std::pow(10.0, 5.0) - std::pow(10.0, 5.0 - 5.0 * y)) / (std::pow(10.0, 5.0) - 1.0);
    case CurveType::FD6: return (std::pow(10.0, 6.0) - std::pow(10.0, 6.0 - 6.0 * y)) / (std::pow(10.0, 6.0) - 1.0);
    case CurveType::FD7: return (std::pow(10.0, 7.0) - std::pow(10.0, 7.0 - 7.0 * y)) / (std::pow(10.0, 7.0) - 1.0);
    case CurveType::FD8: return (std::pow(10.0, 8.0) - std::pow(10.0, 8.0 - 8.0 * y)) / (std::pow(10.0, 8.0) - 1.0);
    case CurveType::FD9: return (std::pow(10.0, 9.0) - std::pow(10.0, 9.0 - 9.0 * y)) / (std::pow(10.0, 9.0) - 1.0);
    case CurveType::FD10: return (std::pow(10.0, 10.0) - std::pow(10.0, 10.0 - 10.0 * y)) / (std::pow(10.0, 10.0) - 1.0);

    case CurveType::Z1: if (y < 0.9) return 8.0 *y / 9.0; else return 2.0 * y - 1.0;
    case CurveType::Z2: if (y < 0.9) return 7.0 *y / 9.0; else return 3.0 * y - 2.0;
    case CurveType::Z3: if (y < 0.9) return 6.0 *y / 9.0; else return 4.0 * y - 3.0;
    case CurveType::Z4: if (y < 0.9) return 5.0 *y / 9.0; else return 5.0 * y - 4.0;
    case CurveType::Z5: if (y < 0.9) return 4.0 *y / 9.0; else return 6.0 * y - 5.0;
    case CurveType::Z6: if (y < 0.9) return 3.0 *y / 9.0; else return 7.0 * y - 6.0;
    case CurveType::Z7: if (y < 0.9) return 2.0 *y / 9.0; else return 8.0 * y - 7.0;
    case CurveType::Z8: if (y < 0.9) return 1.0 *y / 9.0; else return 9.0 * y - 8.0;

    case CurveType::FZ1: if (y < 0.1) return 2.0 *y; else return (8.0 * y + 1.0) / 9.0;
    case CurveType::FZ2: if (y < 0.1) return 3.0 *y; else return (7.0 * y + 2.0) / 9.0;
    case CurveType::FZ3: if (y < 0.1) return 4.0 *y; else return (6.0 * y + 3.0) / 9.0;
    case CurveType::FZ4: if (y < 0.1) return 5.0 *y; else return (5.0 * y + 4.0) / 9.0;
    case CurveType::FZ5: if (y < 0.1) return 6.0 *y; else return (4.0 * y + 5.0) / 9.0;
    case CurveType::FZ6: if (y < 0.1) return 7.0 *y; else return (3.0 * y + 6.0) / 9.0;
    case CurveType::FZ7: if (y < 0.1) return 8.0 *y; else return (2.0 * y + 7.0) / 9.0;
    case CurveType::FZ8: if (y < 0.1) return 9.0 *y; else return (1.0 * y + 8.0) / 9.0;

    case CurveType::Y1: return 1.0 - std::pow(1.0 - y * y, 0.5);
    case CurveType::FY1: return std::pow(2.0 * y - y * y, 0.5);

    case CurveType::S1: return 2.0 * std::asin(y) / pi;
    case CurveType::FS1: return 2.0 * std::acos(1.0 - y) / pi;

    case CurveType::S2: return C_A3_ATAN_COEFF * std::atan(C_A4_ATAN_INNER * (y - 0.5)) + 0.5;
    case CurveType::FS2: return std::tan(C_A2_TAN_COEFF * (y - 0.5)) / C_A1_TAN_DENOM + 0.5;

    case CurveType::H1: return (std::cbrt(2.0 * y - 1.0) + 1.0) / 2.0;
    case CurveType::FH1: if (y < 0.5) return std::cbrt(y / 4.0); else return std::cbrt((y - 1.0) / 4.0) + 1.0;

    case CurveType::SX1: if (maxFreq <= 0.0) return y; return (std::pow(maxFreq / 700.0 + 1.0, y) - 1.0) * (700.0 / maxFreq);
    case CurveType::SX2: return (1960.0 * y) / (maxFreq + 1960.0 - maxFreq * y);

    default: return y;
    }
}