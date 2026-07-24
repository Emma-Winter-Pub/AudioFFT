#pragma once

#include "XStorageBackend.h"

class MacOSStorageBackend : public XStorageBackend {
public:
    VolumeTopology analyze(const QString& filePath) override;
};