#include "StorageUtils.h"

#include <string>
#include <vector>
#include <windows.h>
#include <winioctl.h>

static HANDLE GetVolumeHandleForFile(const std::wstring& filePath) {
    if (filePath.empty()) return INVALID_HANDLE_VALUE;
    if (filePath.length() < 2 || filePath[1] != ':') return INVALID_HANDLE_VALUE;
    std::wstring volumePath = L"\\\\.\\";
    volumePath += filePath.substr(0, 2); 
    HANDLE hDevice = CreateFileW(
        volumePath.c_str(),
        0, 
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    return hDevice;
}

StorageType StorageUtils::detectStorageType(const QString& filePath) {
    std::wstring wPath = filePath.toStdWString();
    HANDLE hDevice = GetVolumeHandleForFile(wPath);    
    if (hDevice == INVALID_HANDLE_VALUE) { 
        return StorageType::Unknown; 
    }
    StorageType result = StorageType::Unknown;
    DWORD bytesReturned = 0;
    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    char buffer[1024] = {};
    if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &buffer, sizeof(buffer), &bytesReturned, NULL)) {
        STORAGE_DEVICE_DESCRIPTOR* devDesc = (STORAGE_DEVICE_DESCRIPTOR*)buffer;
        if (devDesc->BusType == BusTypeUsb) { 
            CloseHandle(hDevice); 
            return StorageType::RemovableDisk; 
        }
    }
    ZeroMemory(&query, sizeof(query));
    query.PropertyId = StorageDeviceSeekPenaltyProperty;
    query.QueryType = PropertyStandardQuery;
    DEVICE_SEEK_PENALTY_DESCRIPTOR seekDesc = {};
    if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &seekDesc, sizeof(seekDesc), &bytesReturned, NULL)) {
        if (seekDesc.IncursSeekPenalty) { 
            result = StorageType::HDD;
        } else { 
            result = StorageType::SSD;
        }
    } else { 
        result = StorageType::Unknown; 
    }
    CloseHandle(hDevice);
    return result;
}

int StorageUtils::getPhysicalDiskID(const QString& filePath) {
    std::wstring wPath = filePath.toStdWString();
    HANDLE hDevice = GetVolumeHandleForFile(wPath);
    if (hDevice == INVALID_HANDLE_VALUE) {
        return -1;
    }
    DWORD bytesReturned = 0;
    const DWORD bufferSize = 1024;
    std::vector<BYTE> buffer(bufferSize);
    BOOL result = DeviceIoControl(
        hDevice,
        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
        NULL,
        0,
        buffer.data(),
        bufferSize,
        &bytesReturned,
        NULL
    );
    CloseHandle(hDevice);
    if (!result) {
        return -1;
    }
    PVOLUME_DISK_EXTENTS pExtents = (PVOLUME_DISK_EXTENTS)buffer.data();
    if (pExtents->NumberOfDiskExtents > 0) {
        return static_cast<int>(pExtents->Extents[0].DiskNumber);
    }
    return -1;
}