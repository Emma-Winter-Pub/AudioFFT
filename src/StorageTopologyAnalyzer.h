#pragma once

#include "StorageTypes.h"

#include <QString>

class StorageTopologyAnalyzer {
public:
    static VolumeTopology analyze(const QString& filePath);

private:
    StorageTopologyAnalyzer() = default;
};