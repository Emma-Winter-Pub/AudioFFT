#include "BatFftProcessor.h"

#include <cmath>
#include <mutex>
#include <fftw3.h>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


static std::mutex g_fftw_plan_mutex;


BatFftProcessor::BatFftProcessor(){}


BatFftProcessor::~BatFftProcessor(){}


std::vector<std::vector<double>> BatFftProcessor::compute(

    const std::vector<float>& pcmData,
    double timeInterval,
    int fftSize,
    int sampleRate)
{
    if (pcmData.size() <= static_cast<size_t>(fftSize) || fftSize <= 0 || sampleRate <= 0 || timeInterval <= 0) {
        return {};
    }

    const int hopSize = std::max(1, static_cast<int>(sampleRate * timeInterval));
    const size_t totalFrames = (pcmData.size() - fftSize) / hopSize;
    if (totalFrames == 0) {
        return {};
    }
    std::vector<double> window(fftSize);
    for (int i = 0; i < fftSize; ++i) {
        window[i] = 0.5 * (1.0 - cos(2.0 * M_PI * i / (fftSize - 1)));
    }

    double* fft_in = (double*)fftw_malloc(sizeof(double) * fftSize);
    fftw_complex* fft_out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (fftSize / 2 + 1));
    if (!fft_in || !fft_out) {
        fftw_free(fft_in);
        fftw_free(fft_out);
        return {};
    }
    
    fftw_plan plan;
    {
        std::lock_guard<std::mutex> lock(g_fftw_plan_mutex);
        plan = fftw_plan_dft_r2c_1d(fftSize, fft_in, fft_out, FFTW_ESTIMATE);
    }

    if (!plan) {
        fftw_free(fft_in);
        fftw_free(fft_out);
        return {};
    }

    std::vector<std::vector<double>> spectrogramData;
    spectrogramData.reserve(totalFrames);

    for (size_t i = 0; i + fftSize <= pcmData.size(); i += hopSize) {
        for (int j = 0; j < fftSize; ++j) {
            fft_in[j] = static_cast<double>(pcmData[i + j]) * window[j];
        }

        fftw_execute(plan);
        
        std::vector<double> column_db(fftSize / 2 + 1);
        for (int k = 0; k < (fftSize / 2 + 1); ++k) {
            double real = fft_out[k][0];
            double imag = fft_out[k][1];
            double magnitude = sqrt(real * real + imag * imag);
            column_db[k] = 20.0 * log10(magnitude / fftSize + 1e-9);
        }
        spectrogramData.push_back(std::move(column_db));
    }

    {
        std::lock_guard<std::mutex> lock(g_fftw_plan_mutex);
        fftw_destroy_plan(plan);
    }

    fftw_free(fft_in);
    fftw_free(fft_out);

    return spectrogramData;
}