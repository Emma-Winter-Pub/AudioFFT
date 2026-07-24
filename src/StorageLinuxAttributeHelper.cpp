#include "StorageLinuxAttributeHelper.h"

#include <QFile>
#include <QFileInfo>

QString LinuxAttributeHelper::readSysfsString(const QString& path) {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(file.readAll()).trimmed();
    }
    return QString();
}

qint64 LinuxAttributeHelper::readSysfsInt(const QString& path, qint64 defaultVal) {
    QString content = readSysfsString(path);
    if (content.isEmpty()) return defaultVal;
    bool ok;
    qint64 val = content.toLongLong(&ok);
    return ok ? val : defaultVal;
}

void LinuxAttributeHelper::resolveAttributes(const QList<ResolvedNode>& leaves, QHash<int, PhysicalDevice>& outDevices) {
    int diskIndex = 0;
    for (const ResolvedNode& node : leaves) {
        if (!QFile::exists(node.sysfsPath)) continue;
        PhysicalDevice phys;
        phys.diskNumber = diskIndex++;
        phys.devicePath = node.devicePath;
        phys.rotational = (readSysfsInt(node.sysfsPath + "/queue/rotational", 1) == 1);
        phys.trimSupported = (readSysfsInt(node.sysfsPath + "/queue/discard_granularity", 0) > 0);
        phys.sizeBytes = readSysfsInt(node.sysfsPath + "/size", 0) * 512;
        phys.vendor = readSysfsString(node.sysfsPath + "/device/vendor");
        phys.model = readSysfsString(node.sysfsPath + "/device/model");
        QString realDevicePath = QFileInfo(node.sysfsPath + "/device").canonicalFilePath();
        if (realDevicePath.contains("/usb")) {
            phys.busType = StorageBusType::USB;
            phys.transport = StorageTransport::USB;
            phys.removable = true;
        } else if (realDevicePath.contains("/nvme")) {
            phys.busType = StorageBusType::NVMe;
            phys.transport = StorageTransport::NVME;
            phys.removable = false;
        } else if (realDevicePath.contains("/ata") || realDevicePath.contains("/scsi")) {
            phys.busType = StorageBusType::SATA; 
            phys.transport = StorageTransport::SATA;
            phys.removable = (readSysfsInt(node.sysfsPath + "/removable", 0) == 1);
        } else {
            phys.busType = StorageBusType::Unknown;
        }
        outDevices.insert(phys.diskNumber, phys);
    }
}