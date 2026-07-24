#pragma once

#include "XStorageBackend.h"

#include <memory>

class StoragePlatformUtils {
public:
    static std::unique_ptr<XStorageBackend> createBackend();
};