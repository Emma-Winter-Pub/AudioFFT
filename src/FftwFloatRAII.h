#pragma once

#include <fftw3.h>
#include <memory>

namespace FftwFloat {
    struct FreeDeleter {
        void operator()(void* p) const { 
            if (p) fftwf_free(p); 
        }
    };
    struct PlanDeleter {
        void operator()(fftwf_plan p) const { 
            if (p) fftwf_destroy_plan(p); 
        }
    };
    using FloatBufferPtr = std::unique_ptr<float, FreeDeleter>;
    using ComplexBufferPtr = std::unique_ptr<fftwf_complex, FreeDeleter>;
    using PlanPtr = std::unique_ptr<std::remove_pointer_t<fftwf_plan>, PlanDeleter>;
    inline FloatBufferPtr allocReal(size_t n) {
        return FloatBufferPtr(static_cast<float*>(fftwf_malloc(sizeof(float) * n)));
    }
    inline ComplexBufferPtr allocComplex(size_t n) {
        return ComplexBufferPtr(static_cast<fftwf_complex*>(fftwf_malloc(sizeof(fftwf_complex) * n)));
    }
}