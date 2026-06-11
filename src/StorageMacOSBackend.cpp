#include "StorageMacOSBackend.h"

VolumeTopology MacOSStorageBackend::analyze(const QString& filePath) {
    VolumeTopology topology;
    topology.originalFilePath = filePath;
    topology.finalNature = DeviceNature::Unknown;
    topology.finalInference = { DeviceNature::Unknown, 0.0f, "macOS backend not fully implemented" };
    return topology;
}