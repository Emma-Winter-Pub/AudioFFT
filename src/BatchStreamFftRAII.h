#pragma once

#include <fftw3.h>
#include <memory>

namespace BatchStreamFftRAII {
    struct FloatFreeDeleter {void operator()(void* p) const { if (p) fftwf_free(p); }};
    struct FloatPlanDeleter {void operator()(fftwf_plan p) const { if (p) fftwf_destroy_plan(p); }};
    using FloatBufferPtr = std::unique_ptr<float, FloatFreeDeleter>;
    using FloatComplexBufferPtr = std::unique_ptr<fftwf_complex, FloatFreeDeleter>;
    using FloatPlanPtr = std::unique_ptr<std::remove_pointer_t<fftwf_plan>, FloatPlanDeleter>;
    inline FloatBufferPtr allocRealFloat(size_t n) {return FloatBufferPtr(static_cast<float*>(fftwf_malloc(sizeof(float) * n)));}
    inline FloatComplexBufferPtr allocComplexFloat(size_t n) {return FloatComplexBufferPtr(static_cast<fftwf_complex*>(fftwf_malloc(sizeof(fftwf_complex) * n)));}
    struct DoubleFreeDeleter {void operator()(void* p) const { if (p) fftw_free(p); }};
    struct DoublePlanDeleter {void operator()(fftw_plan p) const { if (p) fftw_destroy_plan(p); }};
    using DoubleBufferPtr = std::unique_ptr<double, DoubleFreeDeleter>;
    using DoubleComplexBufferPtr = std::unique_ptr<fftw_complex, DoubleFreeDeleter>;
    using DoublePlanPtr = std::unique_ptr<std::remove_pointer_t<fftw_plan>, DoublePlanDeleter>;
    inline DoubleBufferPtr allocRealDouble(size_t n) {return DoubleBufferPtr(static_cast<double*>(fftw_malloc(sizeof(double) * n)));}
    inline DoubleComplexBufferPtr allocComplexDouble(size_t n) {return DoubleComplexBufferPtr(static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * n)));}
}