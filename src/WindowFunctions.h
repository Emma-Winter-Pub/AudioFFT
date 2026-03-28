#pragma once

#include <vector>
#include <QString>
#include <QCoreApplication>

enum class WindowType {
    Rectangular,
    Triangular,
    Hann,
    Hamming,
    Blackman,
    BlackmanHarris,
    BlackmanNuttall,
    Nuttall,
    FlatTop,
    Minimum4Term,
    Minimum7Term,
    Gaussian,
    Kaiser,
    Tukey,
    Lanczos
};

class WindowFunctions {
    Q_DECLARE_TR_FUNCTIONS(WindowFunctions)
public:
    static std::vector<double> generate(int size, WindowType type, double param = 0.0);
    static QString getName(WindowType type, double param = 0.0);
};