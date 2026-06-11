#pragma once

#include "StorageTypes.h"

#include <QString>

class IStorageBackend {
public:
    virtual ~IStorageBackend() = default;
    virtual VolumeTopology analyze(const QString& filePath) = 0;
};