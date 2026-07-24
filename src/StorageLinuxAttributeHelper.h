#pragma once

#include "StorageLinuxTypes.h"

#include <QHash>

class LinuxAttributeHelper {
public:
    static void resolveAttributes(const QList<ResolvedNode>& leaves, QHash<int, PhysicalDevice>& outDevices);
    static QString readSysfsString(const QString& path);
    static qint64 readSysfsInt(const QString& path, qint64 defaultVal = 0);
};