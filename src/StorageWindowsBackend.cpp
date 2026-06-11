#include "StorageWindowsBackend.h"
#include "StorageWindowsIoctlHelper.h"
#include "StorageWindowsWmiHelper.h"
#include "StorageWindowsNetworkHelper.h"

#include <QFileInfo>
#include <windows.h>

VolumeTopology WindowsStorageBackend::analyze(const QString& filePath) {
    VolumeTopology topology;
    topology.originalFilePath = filePath;
    QString driveLetter = filePath.left(2).toUpper();
    if (!driveLetter.endsWith(":")) {
        if (filePath.startsWith("//") || filePath.startsWith("\\\\")) {
            StorageWindowsNetworkHelper::resolveNetworkStorage(topology);
            return topology;
        }
        return topology; 
    }
    std::wstring wDrive = driveLetter.toStdWString() + L"\\";
    UINT driveType = GetDriveTypeW(wDrive.c_str());
    if (driveType == DRIVE_REMOTE) {
        StorageWindowsNetworkHelper::resolveNetworkStorage(topology);
        return topology;
    }
    topology.domain = StorageDomain::LocalPhysical;
    topology.volumePath = "\\\\.\\" + driveLetter;
    topology.extents = StorageWindowsIoctlHelper::getVolumeExtents(topology.volumePath);
    for (const auto& ext : topology.extents) {
        PhysicalDevice device = StorageWindowsIoctlHelper::analyzePhysicalDrive(ext.diskNumber);
        topology.devices.insert(device.diskNumber, device);
    }
    StorageWindowsWmiHelper::enhanceTopology(topology);
    return topology;
}