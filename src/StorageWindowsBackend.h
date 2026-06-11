#pragma once

#include "IStorageBackend.h"

class WindowsStorageBackend : public IStorageBackend {
public:
    ~WindowsStorageBackend() override = default;
    VolumeTopology analyze(const QString& filePath) override;
};