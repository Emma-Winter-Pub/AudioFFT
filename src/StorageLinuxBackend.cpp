#include "StorageLinuxBackend.h"

VolumeTopology LinuxStorageBackend::analyze(const QString& filePath) {
    VolumeTopology topology;
    topology.originalFilePath = filePath;
    topology.finalNature = DeviceNature::Unknown;
    topology.finalInference = { DeviceNature::Unknown, 0.0f, "Linux backend not fully implemented" };
    return topology;
}