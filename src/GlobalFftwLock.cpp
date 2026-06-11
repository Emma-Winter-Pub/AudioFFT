#include "GlobalFftwLock.h"

std::recursive_mutex& getGlobalFftwLock() {
    static std::recursive_mutex s_globalFftwMutex;
    return s_globalFftwMutex;
}