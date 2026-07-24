#pragma once

#include <QString>
#include <QVector>
#include <QHash>
#include <cstdint>

enum class StorageDomain {
    Unknown,
    LocalPhysical,
    LocalVirtual,
    NetworkFileSystem,
    NetworkBlockDevice,
    CloudStorage
};

enum class StorageTransport {
    Unknown,
    SATA,
    NVME,
    USB,
    SAS,
    SCSI,
    SMB,
    NFS,
    WebDAV,
    ISCSI,
    FibreChannel,
    CloudSync
};

enum class StorageBusType {
    Unknown, SCSI, ATAPI, ATA, IEEE1394, SSA, Fibre,
    USB, RAID, iSCSI, SAS, SATA, SD, MMC, Virtual,
    FileBackedVirtual, StorageSpaces, NVMe, UFS
};

enum class DeviceNature {
    Unknown,
    HDD,
    SATA_SSD,
    NVME_SSD,
    USB_SSD,
    Hybrid,
    RAID,
    StorageSpaces,
    VirtualDisk,
    RamDisk,
    NetworkStorage
};

enum class InferenceReasonCode {
    InsufficientEvidence,
    VirtualDiskFlagExplicit,
    NVMeWithTrimAndNoSeekPenalty,
    NVMeBusOnly,
    UsbSsdHeuristics,
    SataSsdHeuristics,
    SataSsdFallback,
    RotationalSeekPenalty,
    NetworkShareDetected,
    StorageSpacesSpanned,
    HardwareRaidDetected,
    HybridDriveTiering,
    SingleDeviceMapping
};

struct StorageInference {
    DeviceNature inferredNature = DeviceNature::Unknown;
    float confidence = 0.0f;
    InferenceReasonCode reasonCode = InferenceReasonCode::InsufficientEvidence;
};

struct PhysicalDevice {
    int diskNumber = -1;
    QString devicePath;
    QString vendor;
    QString model;
    QString serial;
    QString pnpId;
    QString wwn;
    StorageBusType busType = StorageBusType::Unknown;
    StorageTransport transport = StorageTransport::Unknown;
    bool removable = false;
    bool rotational = true;
    bool trimSupported = false;
    bool seekPenalty = true;
    bool isVirtual = false;
    bool isRaidMember = false;
    bool isBootDevice = false;
    bool isSystemDevice = false;
    uint64_t sizeBytes = 0;
    DeviceNature nature = DeviceNature::Unknown;
    StorageInference inference;
};

struct VolumeExtent {
    int diskNumber = -1;
    uint64_t offset = 0;
    uint64_t length = 0;
};

struct NetworkStorageInfo {
    QString uncPath;
    QString server;
    QString share;
    QString provider;
    QString protocol;
    bool offlineFiles = false;
    bool dfs = false;
};

struct VolumeTopology {
    QString originalFilePath;
    QString volumePath;
    QString filesystem;
    StorageDomain domain = StorageDomain::Unknown;
    StorageTransport transport = StorageTransport::Unknown;
    QVector<VolumeExtent> extents;
    QHash<int, PhysicalDevice> devices;
    NetworkStorageInfo networkInfo;
    bool isNetworkStorage = false;
    bool isVirtualDisk = false;
    bool isStorageSpaces = false;
    bool isEncrypted = false;
    bool isRaid = false;
    bool isSpanned = false;
    bool isMirrored = false;
    bool isStriped = false;
    bool isParity = false;
    bool isHybrid = false;
    bool hasSSD = false;
    bool hasHDD = false;
    bool hasNVMe = false;
    bool hasUSB = false;
    bool hasRemoteBackend = false;
    DeviceNature finalNature = DeviceNature::Unknown;
    StorageInference finalInference;
};