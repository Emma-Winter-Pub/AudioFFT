#pragma once

#include "IStorageBackend.h"

class LinuxStorageBackend : public IStorageBackend {
public:
    VolumeTopology analyze(const QString& filePath) override;
};