#include "FFTWGlobalLock.h"

std::recursive_mutex& getFFTWGlobalLock() {
    static std::recursive_mutex s_FFTWGlobalMutex;
    return s_FFTWGlobalMutex;
}