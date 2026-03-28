#pragma once

#include <QString>

enum class StorageType {
    Unknown,
    HDD,
    SSD,
    RemovableDisk
};

class StorageUtils {
public:
    static StorageType detectStorageType(const QString& filePath);
    static int getPhysicalDiskID(const QString& filePath);
};