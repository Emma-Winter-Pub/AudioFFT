#ifndef GLOBAL_FFTW_LOCK_H
#define GLOBAL_FFTW_LOCK_H

#include <mutex>

std::recursive_mutex& getGlobalFftwLock();

#endif