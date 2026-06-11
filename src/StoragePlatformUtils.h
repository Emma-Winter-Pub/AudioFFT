#pragma once

#include "IStorageBackend.h"

#include <memory>

class StoragePlatformUtils {
public:
    static std::unique_ptr<IStorageBackend> createBackend();
};