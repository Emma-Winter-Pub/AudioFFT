#pragma once

#include "StorageTypes.h"

#include <QString>

class XStorageBackend {
public:
    virtual ~XStorageBackend() = default;
    virtual VolumeTopology analyze(const QString& filePath) = 0;
};