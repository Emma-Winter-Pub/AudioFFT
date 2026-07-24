#include "StoragePlatformUtils.h"

#if defined(_WIN32)
    #include "StorageWindowsBackend.h"
#elif defined(__linux__)
    #include "StorageLinuxBackend.h"
#elif defined(__APPLE__) && defined(__MACH__)
    #include "StorageMacOSBackend.h"
#endif

std::unique_ptr<XStorageBackend> StoragePlatformUtils::createBackend() {
#if defined(_WIN32)
    return std::make_unique<WindowsStorageBackend>();
#elif defined(__linux__)
    return std::make_unique<LinuxStorageBackend>();
#elif defined(__APPLE__) && defined(__MACH__)
    return std::make_unique<MacOSStorageBackend>();
#else
    return nullptr;
#endif
}