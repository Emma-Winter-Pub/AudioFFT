#pragma once

#include <vector>
#include <QString>
#include <QCoreApplication>

enum class FFTWindowType {
    Rectangular,
    Triangular,
    Hann,
    Hamming,
    Blackman,
    BlackmanHarris,
    FlatTop,
    Sine,
    Cauchy,
    Parzen,
    Poisson,
    Bohman,
    Nuttall,
    Lanczos,
    Welch,
    DolphChebyshev,
    BartlettHann,
    Minimum4Term,
    Minimum7Term,
    Gaussian,
    Kaiser,
    Tukey
};

class FFTWindowFunctions {
    Q_DECLARE_TR_FUNCTIONS(FFTWindowFunctions)
public:
    static std::vector<double> generate(int size, FFTWindowType type, double param = 0.0);
    static QString getName(FFTWindowType type, double param = 0.0);
};