#pragma once

#include "StorageTypes.h"

#include <QString>
#include <QVector>

class StorageWindowsIoctlHelper {
public:
    static QVector<VolumeExtent> getVolumeExtents(const QString& volumePath);
    static PhysicalDevice analyzePhysicalDrive(int diskNumber);

private:
    static StorageBusType mapBusType(int winBusType);
};