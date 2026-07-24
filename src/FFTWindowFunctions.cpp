#include "FFTWindowFunctions.h"

#include <cmath>
#include <vector>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr double pi = M_PI;

std::vector<double> FFTWindowFunctions::generate(int size, FFTWindowType type, double param) {
    if (size <= 0) return {};
    if (size == 1) return {1.0};
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
    if (type == FFTWindowType::DolphChebyshev) {
        double b_cosh = std::cosh(7.6009022095419887 / Nm1);
        double c = 1.0 - 1.0 / (b_cosh * b_cosh);
        double norm = 0.0;
        for (int n_idx = static_cast<int>(Nm1) / 2; n_idx >= 0; --n_idx) {
            double sum = (n_idx == 0) ? 1.0 : 0.0;
            double b_val = 1.0;
            double t = 1.0;
            for (int j = 1; j <= n_idx && sum != t; ) {
                t = sum;
                b_val *= c * (size - n_idx - j) / j;
                sum += b_val;
                b_val *= (n_idx - j) / static_cast<double>(j);
                ++j;
            }
            sum /= (size - 1 - n_idx);
            if (norm == 0.0) norm = sum;
            sum /= norm;
            w[n_idx] = sum;
            w[size - 1 - n_idx] = sum;
        }
        return w;
    }
    for (int n = 0; n < size; ++n) {
        double val = 1.0;
        switch (type) {
            case FFTWindowType::Rectangular:
                val = 1.0;
                break;
            case FFTWindowType::Triangular:
                val = 1.0 - std::abs((n - Nm1/2.0) / (Nm1/2.0));
                break;
            case FFTWindowType::Hann:
                val = 0.5 * (1.0 - cosTerm(1, n));
                break;
            case FFTWindowType::Hamming:
                val = 0.54 - 0.46 * cosTerm(1, n);
                break;
            case FFTWindowType::Blackman:
                val = 0.42659 - 0.49656 * cosTerm(1, n) + 0.076849 * cosTerm(2, n);
                break;
            case FFTWindowType::BlackmanHarris:
                val = 0.35875 - 0.48829 * cosTerm(1, n) + 0.14128 * cosTerm(2, n) - 0.01168 * cosTerm(3, n);
                break;
            case FFTWindowType::Nuttall:
                val = 0.355768 - 0.487396 * cosTerm(1, n) + 0.144232 * cosTerm(2, n) - 0.012604 * cosTerm(3, n);
                break;
            case FFTWindowType::FlatTop:
                val = 1.0
                    - 1.985844164102 * cosTerm(1, n)
                    + 1.791176438506 * cosTerm(2, n)
                    - 1.282075284005 * cosTerm(3, n)
                    + 0.667777530266 * cosTerm(4, n)
                    - 0.240160796576 * cosTerm(5, n)
                    + 0.056656381764 * cosTerm(6, n)
                    - 0.008134974479 * cosTerm(7, n)
                    + 0.000624544650 * cosTerm(8, n)
                    - 0.000019808998 * cosTerm(9, n)
                    + 0.000000132974 * cosTerm(10, n);
                break;
            case FFTWindowType::Gaussian:
                {
                    double sigma = (param <= 0.0) ? 0.45 : param;
                    double a = (n - Nm1/2.0) / (sigma * Nm1/2.0);
                    val = std::exp(-0.5 * a * a);
                }
                break;
            case FFTWindowType::Kaiser:
                {
                    double beta = (param <= 0.0) ? 8.6 : param;
                    double half = Nm1 / 2.0;
                    double r = (n - half) / half;
                    double arg = 1.0 - r * r;
                    if (arg < 0) arg = 0;
                    val = i0(beta * std::sqrt(arg)) / i0(beta);
                }
                break;
            case FFTWindowType::Tukey:
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
            case FFTWindowType::Minimum4Term:
                val = 0.3635819 
                    - 0.4891775 * cosTerm(1, n) 
                    + 0.1365995 * cosTerm(2, n) 
                    - 0.0106411 * cosTerm(3, n);
                break;
            case FFTWindowType::Minimum7Term:
                val = 0.27105140069342415
                    - 0.43329793923448327 * cosTerm(1,n)
                    + 0.21812299954311063 * cosTerm(2,n)
                    - 0.065925446388030858 * cosTerm(3,n)
                    + 0.010811742098309682 * cosTerm(4,n)
                    - 0.00077658482522130464 * cosTerm(5,n)
                    + 1.8632870148749191e-05 * cosTerm(6,n);
                break;
            case FFTWindowType::Lanczos:
                {
                    double x = 2.0 * n / Nm1 - 1.0;
                    if (std::abs(x) < 1e-9) val = 1.0;
                    else val = std::sin(pi * x) / (pi * x);
                }
                break;
            case FFTWindowType::Welch:
                {
                    double x = (n - Nm1/2.0) / (Nm1/2.0);
                    val = 1.0 - x * x;
                }
                break;
            case FFTWindowType::BartlettHann:
                {
                    double x = static_cast<double>(n) / Nm1;
                    val = 0.62 - 0.48 * std::abs(x - 0.5) - 0.38 * cosTerm(1, n);
                }
                break;
            case FFTWindowType::Sine:
                val = std::sin(pi * n / Nm1);
                break;
            case FFTWindowType::Cauchy:
                {
                    double x = 2.0 * (n / Nm1 - 0.5);
                    if (x <= -0.5 || x >= 0.5) val = 0.0;
                    else val = std::min(1.0, std::abs(1.0 / (1.0 + 64.0 * x * x)));
                }
                break;
            case FFTWindowType::Parzen:
                {
                    double x = 2.0 * (n / Nm1 - 0.5);
                    if (x > 0.25 && x <= 0.5) val = -2.0 * std::pow(-1.0 + 2.0 * x, 3.0);
                    else if (x >= -0.5 && x < -0.25) val = 2.0 * std::pow(1.0 + 2.0 * x, 3.0);
                    else if (x >= -0.25 && x < 0.0) val = 1.0 - 24.0 * x * x - 48.0 * x * x * x;
                    else if (x >= 0.0 && x <= 0.25) val = 1.0 - 24.0 * x * x + 48.0 * x * x * x;
                    else val = 0.0;
                }
                break;
            case FFTWindowType::Poisson:
                {
                    double x = 2.0 * (n / Nm1 - 0.5);
                    if (x >= 0.0 && x <= 0.5) val = std::exp(-6.0 * x);
                    else if (x < 0.0 && x >= -0.5) val = std::exp(6.0 * x);
                    else val = 0.0;
                }
                break;
            case FFTWindowType::Bohman:
                {
                    double x = 2.0 * (n / Nm1) - 1.0;
                    val = (1.0 - std::abs(x)) * std::cos(pi * std::abs(x)) + (1.0 / pi) * std::sin(pi * std::abs(x));
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

QString FFTWindowFunctions::getName(FFTWindowType type, double param) {
    switch (type) {
        case FFTWindowType::Rectangular:      return tr("矩形");
        case FFTWindowType::Triangular:       return tr("三角");
        case FFTWindowType::Hann:             return tr("汉宁");
        case FFTWindowType::Hamming:          return tr("汉明");
        case FFTWindowType::Blackman:         return tr("黑曼");
        case FFTWindowType::BlackmanHarris:   return tr("黑哈");
        case FFTWindowType::FlatTop:          return tr("平顶");
        case FFTWindowType::Sine:             return tr("正弦");
        case FFTWindowType::Cauchy:           return tr("柯西");
        case FFTWindowType::Parzen:           return tr("帕森");
        case FFTWindowType::Poisson:          return tr("泊松");
        case FFTWindowType::Bohman:           return tr("博曼");
        case FFTWindowType::Nuttall:          return tr("纳特尔");
        case FFTWindowType::Lanczos:          return tr("兰索斯");
        case FFTWindowType::Welch:            return tr("韦尔奇");
        case FFTWindowType::DolphChebyshev:   return tr("切比雪夫");
        case FFTWindowType::BartlettHann:     return tr("三角汉宁");
        case FFTWindowType::Minimum4Term:     return tr("最小四项");
        case FFTWindowType::Minimum7Term:     return tr("最小七项");
        case FFTWindowType::Gaussian:         return tr("高斯(σ=%1)").arg((param<=0?0.45:param),0,'f',2);
        case FFTWindowType::Kaiser:           return tr("凯泽(β=%1)").arg((param<=0?8.6:param),0,'f',1);
        case FFTWindowType::Tukey:            return tr("图基(α=%1)").arg((param<=0?0.5:param),0,'f',1);
        default:                           return tr("汉宁");
    }
}