#include "FftProcessor.h"
#include "AppConfig.h"
#include "Utils.h"

#include <cmath>
#include <vector>
#include <thread>
#include <memory>
#include <stdexcept>
#include <fftw3.h>


namespace {

struct FftwMemoryDeleter   { void operator()(void* ptr)   const { if (ptr) fftw_free(ptr)    ; } };
struct FftwPlanDeleter     { void operator()(fftw_plan p) const { if (p) fftw_destroy_plan(p); } };
using  FftwDoubleArrayPtr  = std::unique_ptr<double[], FftwMemoryDeleter>;
using  FftwComplexArrayPtr = std::unique_ptr<fftw_complex[], FftwMemoryDeleter>;
using  FftwPlanPtr         = std::unique_ptr<fftw_plan_s, FftwPlanDeleter>;

struct FftThreadContext {
    FftwPlanPtr plan;
    FftwDoubleArrayPtr fft_in;
    FftwComplexArrayPtr fft_out;
};

void fft_worker(
    const std::vector<float>* pcmData,
    const std::vector<double>* window,
    std::vector<std::vector<double>>* resultData,
    FftThreadContext* thread_context,
    size_t start_frame,
    size_t end_frame,
    int hop_size,
    int fft_size,
    int fft_out_size
) {
    for (size_t frame_idx = start_frame; frame_idx < end_frame; ++frame_idx) {
        size_t start_sample_idx = frame_idx * hop_size;

        for (int j = 0; j < fft_size; ++j) {
            thread_context->fft_in[j] = static_cast<double>((*pcmData)[start_sample_idx + j]) * (*window)[j];
        }

        fftw_execute(thread_context->plan.get());
        
        auto& db_column = (*resultData)[frame_idx];
        for (int k = 0; k < fft_out_size; ++k) {
            double real = thread_context->fft_out[k][0];
            double imag = thread_context->fft_out[k][1];
            double magnitude = sqrt(real * real + imag * imag);
            db_column[k] = 20.0 * log10(magnitude / fft_size + 1e-9);
        }
    }
}

} // namespace


FftProcessor::FftProcessor(QObject *parent) : QObject(parent) {}


bool FftProcessor::compute(const std::vector<float>& pcmData, std::vector<std::vector<double>>& spectrogramData, double timeInterval, int fftSize, int nativeSampleRate){

    emit logMessage(getCurrentTimestamp() + tr(" Starting FFT"));
    emit logMessage(QString(tr("                        FFT Size: %1, Sample Rate: %2 Hz")).arg(fftSize).arg(nativeSampleRate));
    
    const int HOP_SIZE = std::max(1, static_cast<int>(nativeSampleRate * timeInterval));
    emit logMessage(QString(tr("                        Time Precision: %1s, Step: %2 Hz")).arg(timeInterval).arg(HOP_SIZE));

    if (pcmData.size() <= static_cast<size_t>(fftSize)) {
        return false;}

    const size_t totalFrames = (pcmData.size() - fftSize) / HOP_SIZE;
    spectrogramData.clear();

    if (totalFrames == 0) {
        return false;}

    if (totalFrames < CoreConfig::FFT_MULTITHREAD_THRESHOLD) {
        emit logMessage(QString(tr("                        Using single-threaded calculation")));
        try {
            std::vector<double> window(fftSize);
            for (int i = 0; i < fftSize; ++i) {
                window[i] = 0.5 * (1 - cos(2 * M_PI * i / (fftSize - 1)));}

            const int fft_out_size = fftSize / 2 + 1;
            FftwDoubleArrayPtr fft_in(static_cast<double*>(fftw_malloc(sizeof(double) * fftSize)));
            FftwComplexArrayPtr fft_out(static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * fft_out_size)));
            if (!fft_in || !fft_out) { throw std::runtime_error("Failed to allocate FFTW memory."); }

            FftwPlanPtr plan(fftw_plan_dft_r2c_1d(fftSize, fft_in.get(), fft_out.get(), FFTW_ESTIMATE));
            if (!plan) { throw std::runtime_error("Failed to create FFTW plan."); }
            
            spectrogramData.reserve(totalFrames);
            for (size_t i = 0; i + fftSize <= pcmData.size(); i += HOP_SIZE) {
                for (int j = 0; j < fftSize; ++j) {
                    fft_in[j] = static_cast<double>(pcmData[i + j]) * window[j];
                }
                fftw_execute(plan.get());
                std::vector<double> column_db(fft_out_size);
                for (int k = 0; k < fft_out_size; ++k) {
                    double real = fft_out[k][0];
                    double imag = fft_out[k][1];
                    double magnitude = sqrt(real * real + imag * imag);
                    column_db[k] = 20.0 * log10(magnitude / fftSize + 1e-9);
                }
                spectrogramData.push_back(std::move(column_db));}

        } catch (const std::exception& e) {
            emit logMessage(QString(tr("                        Single-threaded FFT failed: %1")).arg(e.what()));
            spectrogramData.clear();
            return false;}

    } else {
        unsigned int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 1;

        emit logMessage(QString(tr("                        Using %1 threads for calculation")).arg(numThreads));

        try {
            std::vector<double> window(fftSize);
            for (int i = 0; i < fftSize; ++i) {
                window[i] = 0.5 * (1 - cos(2 * M_PI * i / (fftSize - 1)));
            }

            const int fft_out_size = fftSize / 2 + 1;
            spectrogramData.assign(totalFrames, std::vector<double>(fft_out_size));

            std::vector<FftThreadContext> thread_contexts(numThreads);
            for (unsigned int i = 0; i < numThreads; ++i) {
                thread_contexts[i].fft_in.reset(static_cast<double*>(fftw_malloc(sizeof(double) * fftSize)));
                thread_contexts[i].fft_out.reset(static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * fft_out_size)));
                if (!thread_contexts[i].fft_in || !thread_contexts[i].fft_out) {
                    throw std::runtime_error("Failed to allocate FFTW memory for thread.");}

                thread_contexts[i].plan.reset(fftw_plan_dft_r2c_1d(fftSize, thread_contexts[i].fft_in.get(), thread_contexts[i].fft_out.get(), FFTW_ESTIMATE));
                if (!thread_contexts[i].plan) {
                    throw std::runtime_error("Failed to create FFTW plan for thread.");}}

            std::vector<std::thread> threads;
            size_t frames_per_thread = totalFrames / numThreads;

            for (unsigned int i = 0; i < numThreads; ++i) {
                size_t start_frame = i * frames_per_thread;
                size_t end_frame = (i == numThreads - 1) ? totalFrames : start_frame + frames_per_thread;
                
                if (start_frame < end_frame) {
                    threads.emplace_back(fft_worker, &pcmData, &window, &spectrogramData,
                                         &thread_contexts[i],
                                         start_frame, end_frame, HOP_SIZE, fftSize, fft_out_size);}}
            
            for(auto& t : threads) {
                if (t.joinable()) {
                    t.join();}}

        } catch (const std::exception& e) {
            emit logMessage(QString(tr("                        Multi-threaded FFT failed: %1")).arg(e.what()));
            spectrogramData.clear();
            return false;}}
    
    return !spectrogramData.empty();
}