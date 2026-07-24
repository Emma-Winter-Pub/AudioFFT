#pragma once

#include "XStorageBackend.h"

class WindowsStorageBackend : public XStorageBackend {
public:
    ~WindowsStorageBackend() override = default;
    VolumeTopology analyze(const QString& filePath) override;
};