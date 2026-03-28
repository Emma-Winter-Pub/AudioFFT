#include "WindowFunctions.h"

#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr double pi = M_PI;

std::vector<double> WindowFunctions::generate(int size, WindowType type, double param) {
    if (size <= 0) return {};
    std::vector<double> w(size);
    const double N  = static_cast<double>(size);
    const double Nm1 = N - 1.0;
    auto cosTerm = [&](int k, int n) {
        return std::cos(2.0 * pi * k * n / Nm1);
    };
    auto i0 = [](double x) -> double {
        double sum = 1.0;
        double term = 1.0;
        for (int k = 1; k <= 20; ++k) {
            term *= (x * x) / (4.0 * k * k);
            sum += term;
            if (term < 1e-18 * sum) break;
        }
        return sum;
    };
    for (int n = 0; n < size; ++n) {
        double val = 1.0;
        switch (type) {
            case WindowType::Rectangular:
                val = 1.0;
                break;
            case WindowType::Triangular:
                val = 1.0 - std::abs((n - Nm1/2.0) / (Nm1/2.0));
                break;
            case WindowType::Hann:
                val = 0.5 * (1.0 - cosTerm(1, n));
                break;
            case WindowType::Hamming:
                val = 0.54 - 0.46 * cosTerm(1, n);
                break;
            case WindowType::Blackman:
                val = 0.42 - 0.50 * cosTerm(1, n) + 0.08 * cosTerm(2, n);
                break;
            case WindowType::BlackmanHarris:
                val = 0.35875 - 0.48829 * cosTerm(1, n) + 0.14128 * cosTerm(2, n) - 0.01168 * cosTerm(3, n);
                break;
            case WindowType::BlackmanNuttall:
                val = 0.3635819 - 0.4891775 * cosTerm(1, n) + 0.1365995 * cosTerm(2, n) - 0.0106411 * cosTerm(3, n);
                break;
            case WindowType::Nuttall:
                val = 0.355768 - 0.487396 * cosTerm(1, n) + 0.144232 * cosTerm(2, n) - 0.012604 * cosTerm(3, n);
                break;
            case WindowType::FlatTop:
                val = 0.21557895
                    - 0.41663158 * cosTerm(1, n)
                    + 0.277263158 * cosTerm(2, n)
                    - 0.083578947 * cosTerm(3, n)
                    + 0.006947368 * cosTerm(4, n);
                break;
            case WindowType::Gaussian:
                {
                    double sigma = (param <= 0.0) ? 0.45 : param;
                    double a = (n - Nm1/2.0) / (sigma * Nm1/2.0);
                    val = std::exp(-0.5 * a * a);
                }
                break;
            case WindowType::Kaiser:
                {
                    double beta = (param <= 0.0) ? 8.6 : param;
                    double half = Nm1 / 2.0;
                    double r = (n - half) / half;
                    double arg = 1.0 - r * r;
                    if (arg < 0) arg = 0;
                    
                    double num = i0(beta * std::sqrt(arg));
                    double den = i0(beta);
                    val = num / den;
                }
                break;
            case WindowType::Tukey:
                {
                    double alpha = (param <= 0.0 || param > 1.0) ? 0.5 : param;
                    double N_alpha = alpha * Nm1;
                    if (n < N_alpha / 2.0) {
                        val = 0.5 * (1.0 - std::cos(2.0 * pi * n / N_alpha));
                    } else if (n > Nm1 - N_alpha / 2.0) {
                        val = 0.5 * (1.0 - std::cos(2.0 * pi * (Nm1 - n) / N_alpha));
                    } else {
                        val = 1.0;
                    }
                }
                break;
            case WindowType::Minimum4Term:
                val = 0.35875 - 0.48829 * cosTerm(1,n) + 0.14128 * cosTerm(2,n) - 0.01168 * cosTerm(3,n);
                break;
            case WindowType::Minimum7Term:
                val = 0.27105140069342415
                    - 0.43329793923448327 * cosTerm(1,n)
                    + 0.21812299954311063 * cosTerm(2,n)
                    - 0.065925446388030858 * cosTerm(3,n)
                    + 0.010811742098309682 * cosTerm(4,n)
                    - 0.00077658482522130464 * cosTerm(5,n)
                    + 1.8632870148749191e-05 * cosTerm(6,n);
                break;
            case WindowType::Lanczos:
                {
                    double x = 2.0 * n / Nm1 - 1.0;
                    if (std::abs(x) < 1e-9) val = 1.0;
                    else val = std::sin(pi * x) / (pi * x);
                }
                break;
            default:
                val = 1.0;
                break;
        }
        w[n] = val;
    }
    return w;
}

QString WindowFunctions::getName(WindowType type, double param) {
    switch (type) {
        case WindowType::Rectangular:      return tr("矩形");
        case WindowType::Triangular:       return tr("三角");
        case WindowType::Hann:             return tr("汉宁");
        case WindowType::Hamming:          return tr("汉明");
        case WindowType::Blackman:         return tr("黑曼");
        case WindowType::BlackmanHarris:   return tr("布哈");
        case WindowType::BlackmanNuttall:  return tr("布纳");
        case WindowType::Nuttall:          return tr("纳特尔");
        case WindowType::FlatTop:          return tr("平顶");
        case WindowType::Minimum4Term:     return tr("最小4项");
        case WindowType::Minimum7Term:     return tr("最小7项");
        case WindowType::Gaussian:         return tr("高斯(σ=%1)").arg((param<=0?0.45:param),0,'f',2);
        case WindowType::Kaiser:           return tr("凯泽(β=%1)").arg((param<=0?8.6:param),0,'f',1);
        case WindowType::Tukey:            return tr("图基(α=%1)").arg((param<=0?0.5:param),0,'f',1);
        case WindowType::Lanczos:          return tr("兰索斯");
        default:                           return tr("汉宁");
    }
}