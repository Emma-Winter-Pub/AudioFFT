#include "StorageTopologyAnalyzer.h"
#include "StoragePlatformUtils.h"
#include "StorageCache.h"
#include "StorageClassifier.h"

#include <QFileInfo>
#include <QStorageInfo>

VolumeTopology StorageTopologyAnalyzer::analyze(const QString& filePath) {
    VolumeTopology topology;
    topology.originalFilePath = filePath;
    QFileInfo fileInfo(filePath);
    QString absPath = fileInfo.absoluteFilePath();
    QStorageInfo storageInfo(absPath);
    QString cacheKey = storageInfo.rootPath();
    if (cacheKey.isEmpty()) {
        cacheKey = fileInfo.absolutePath();
    }
    if (StorageCache::instance().tryGet(cacheKey, topology)) {
        topology.originalFilePath = absPath;
        return topology;
    }
    auto backend = StoragePlatformUtils::createBackend();
    if (!backend) {
        topology.finalNature = DeviceNature::Unknown;
        topology.finalInference = { DeviceNature::Unknown, 0.0f, InferenceReasonCode::InsufficientEvidence };
        return topology;
    }
    topology = backend->analyze(absPath);
    if (topology.volumePath.isEmpty()) {
        topology.volumePath = cacheKey;
    }
    StorageClassifier::synthesizeTopology(topology);
    StorageCache::instance().insert(cacheKey, topology);
    return topology;
}