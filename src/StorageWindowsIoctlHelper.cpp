#include "StorageWindowsIoctlHelper.h"

#include <windows.h>
#include <winioctl.h>
#include <vector>
#include <cstddef>

QVector<VolumeExtent> StorageWindowsIoctlHelper::getVolumeExtents(const QString& volumePath) {
    QVector<VolumeExtent> extents;
    std::wstring wPath = volumePath.toStdWString();
    HANDLE hVolume = CreateFileW(wPath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                 NULL, OPEN_EXISTING, 0, NULL);
    if (hVolume == INVALID_HANDLE_VALUE) {
        return extents;
    }
    DWORD bytesReturned = 0;
    DWORD bufSize = offsetof(VOLUME_DISK_EXTENTS, Extents) + sizeof(DISK_EXTENT);
    std::vector<BYTE> buffer(bufSize, 0);
    BOOL success = DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, 
                                   NULL, 0, buffer.data(), bufSize, &bytesReturned, NULL);
    if (!success && GetLastError() == ERROR_MORE_DATA) {
        PVOLUME_DISK_EXTENTS pExtents = reinterpret_cast<PVOLUME_DISK_EXTENTS>(buffer.data());
        DWORD requiredExtents = pExtents->NumberOfDiskExtents;
        bufSize = offsetof(VOLUME_DISK_EXTENTS, Extents) + requiredExtents * sizeof(DISK_EXTENT);
        buffer.resize(bufSize);
        success = DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, 
                                  NULL, 0, buffer.data(), bufSize, &bytesReturned, NULL);
    }
    if (success) {
        PVOLUME_DISK_EXTENTS pExtents = reinterpret_cast<PVOLUME_DISK_EXTENTS>(buffer.data());
        for (DWORD i = 0; i < pExtents->NumberOfDiskExtents; ++i) {
            VolumeExtent ve;
            ve.diskNumber = static_cast<int>(pExtents->Extents[i].DiskNumber);
            ve.offset = pExtents->Extents[i].StartingOffset.QuadPart;
            ve.length = pExtents->Extents[i].ExtentLength.QuadPart;
            extents.push_back(ve);
        }
    }
    CloseHandle(hVolume);
    return extents;
}

static bool QueryDynamicStorageProperty(HANDLE hDevice, STORAGE_PROPERTY_ID propertyId, std::vector<BYTE>& outBuffer) {
    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = propertyId;
    query.QueryType = PropertyStandardQuery;
    STORAGE_DESCRIPTOR_HEADER header = {};
    DWORD bytesReturned = 0;
    BOOL success = DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, 
                                   &query, sizeof(query), 
                                   &header, sizeof(header), 
                                   &bytesReturned, NULL);
    if (header.Size == 0) {
        return false;
    }
    outBuffer.resize(header.Size, 0);
    success = DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, 
                              &query, sizeof(query), 
                              outBuffer.data(), header.Size, 
                              &bytesReturned, NULL);
    return success == TRUE;
}

template<typename T>
static bool QueryFixedStorageProperty(HANDLE hDevice, STORAGE_PROPERTY_ID propertyId, T& outStruct) {
    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = propertyId;
    query.QueryType = PropertyStandardQuery;
    DWORD bytesReturned = 0;
    return DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), 
                           &outStruct, sizeof(outStruct), &bytesReturned, NULL);
}

PhysicalDevice StorageWindowsIoctlHelper::analyzePhysicalDrive(int diskNumber) {
    PhysicalDevice device;
    device.diskNumber = diskNumber;
    device.devicePath = QString("\\\\.\\PhysicalDrive%1").arg(diskNumber);
    std::wstring wPath = device.devicePath.toStdWString();
    HANDLE hDevice = CreateFileW(wPath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                 NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        return device;
    }
    DEVICE_SEEK_PENALTY_DESCRIPTOR seekDesc = {};
    if (QueryFixedStorageProperty(hDevice, StorageDeviceSeekPenaltyProperty, seekDesc)) {
        device.seekPenalty = seekDesc.IncursSeekPenalty;
        device.rotational = seekDesc.IncursSeekPenalty;
    }
    DEVICE_TRIM_DESCRIPTOR trimDesc = {};
    if (QueryFixedStorageProperty(hDevice, StorageDeviceTrimProperty, trimDesc)) {
        device.trimSupported = trimDesc.TrimEnabled;
    }
    std::vector<BYTE> devBuffer;
    if (QueryDynamicStorageProperty(hDevice, StorageDeviceProperty, devBuffer)) {
        PSTORAGE_DEVICE_DESCRIPTOR devDesc = reinterpret_cast<PSTORAGE_DEVICE_DESCRIPTOR>(devBuffer.data());
        device.removable = devDesc->RemovableMedia;
        device.busType = mapBusType(devDesc->BusType);
        if (devDesc->VendorIdOffset != 0) {
            device.vendor = QString::fromLocal8Bit(reinterpret_cast<const char*>(devBuffer.data() + devDesc->VendorIdOffset)).trimmed();
        }
        if (devDesc->ProductIdOffset != 0) {
            device.model = QString::fromLocal8Bit(reinterpret_cast<const char*>(devBuffer.data() + devDesc->ProductIdOffset)).trimmed();
        }
        if (devDesc->SerialNumberOffset != 0) {
            device.serial = QString::fromLocal8Bit(reinterpret_cast<const char*>(devBuffer.data() + devDesc->SerialNumberOffset)).trimmed();
        }
    }
    DISK_GEOMETRY_EX geom = {};
    DWORD bytesReturned = 0;
    if (DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, 
                        &geom, sizeof(geom), &bytesReturned, NULL)) {
        device.sizeBytes = geom.DiskSize.QuadPart;
    }
    CloseHandle(hDevice);
    return device;
}

StorageBusType StorageWindowsIoctlHelper::mapBusType(int winBusType) {
    switch (winBusType) {
        case BusTypeScsi:               return StorageBusType::SCSI;
        case BusTypeAtapi:              return StorageBusType::ATAPI;
        case BusTypeAta:                return StorageBusType::ATA;
        case BusType1394:               return StorageBusType::IEEE1394;
        case BusTypeSsa:                return StorageBusType::SSA;
        case BusTypeFibre:              return StorageBusType::Fibre;
        case BusTypeUsb:                return StorageBusType::USB;
        case BusTypeRAID:               return StorageBusType::RAID;
        case BusTypeiScsi:              return StorageBusType::iSCSI;
        case BusTypeSas:                return StorageBusType::SAS;
        case BusTypeSata:               return StorageBusType::SATA;
        case BusTypeSd:                 return StorageBusType::SD;
        case BusTypeMmc:                return StorageBusType::MMC;
        case BusTypeVirtual:            return StorageBusType::Virtual;
        case BusTypeFileBackedVirtual:  return StorageBusType::FileBackedVirtual;
        case BusTypeSpaces:             return StorageBusType::StorageSpaces;
        case BusTypeNvme:               return StorageBusType::NVMe;
        case BusTypeUfs:                return StorageBusType::UFS;
        default:                        return StorageBusType::Unknown;
    }
}