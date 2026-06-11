#pragma once

#include "IStorageBackend.h"

class MacOSStorageBackend : public IStorageBackend {
public:
    VolumeTopology analyze(const QString& filePath) override;
};