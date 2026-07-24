#pragma once

#include <fftw3.h>
#include <memory>

namespace FftwDouble {
    struct FreeDeleter {
        void operator()(void* p) const { 
            if (p) fftw_free(p); 
        }
    };
    struct PlanDeleter {
        void operator()(fftw_plan p) const { 
            if (p) fftw_destroy_plan(p); 
        }
    };
    using DoubleBufferPtr = std::unique_ptr<double, FreeDeleter>;
    using ComplexBufferPtr = std::unique_ptr<fftw_complex, FreeDeleter>;
    using PlanPtr = std::unique_ptr<std::remove_pointer_t<fftw_plan>, PlanDeleter>;
    inline DoubleBufferPtr allocReal(size_t n) {
        return DoubleBufferPtr(static_cast<double*>(fftw_malloc(sizeof(double) * n)));
    }
    inline ComplexBufferPtr allocComplex(size_t n) {
        return ComplexBufferPtr(static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * n)));
    }
}