#pragma once

#include "XStorageBackend.h"

class LinuxStorageBackend : public XStorageBackend {
public:
    ~LinuxStorageBackend() override = default;
    VolumeTopology analyze(const QString& filePath) override;
};