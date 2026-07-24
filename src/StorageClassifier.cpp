#include "StorageClassifier.h"
#include <algorithm>

void StorageClassifier::classifySingleDevice(PhysicalDevice& device) {
    if (device.isVirtual || device.busType == StorageBusType::Virtual || device.busType == StorageBusType::FileBackedVirtual) {
        device.nature = DeviceNature::VirtualDisk;
        device.inference = {DeviceNature::VirtualDisk, 0.95f, InferenceReasonCode::VirtualDiskFlagExplicit};
        return;
    }
    float ssdScore = 0.0f;
    if (device.busType == StorageBusType::NVMe) ssdScore += 0.5f;
    if (device.trimSupported) ssdScore += 0.2f;
    if (!device.seekPenalty)  ssdScore += 0.2f;
    if (device.busType == StorageBusType::USB)  ssdScore -= 0.1f;
    if (device.seekPenalty)   ssdScore -= 0.5f;
    if (ssdScore >= 0.35f) {
        if (device.busType == StorageBusType::NVMe) {
            device.nature = DeviceNature::NVME_SSD;
            device.inference = {DeviceNature::NVME_SSD, std::min(0.99f, ssdScore + 0.2f), InferenceReasonCode::NVMeWithTrimAndNoSeekPenalty};
        } else if (device.busType == StorageBusType::USB) {
            device.nature = DeviceNature::USB_SSD;
            device.inference = {DeviceNature::USB_SSD, std::min(0.95f, ssdScore + 0.1f), InferenceReasonCode::UsbSsdHeuristics};
        } else {
            device.nature = DeviceNature::SATA_SSD;
            device.inference = {DeviceNature::SATA_SSD, std::min(0.90f, ssdScore), InferenceReasonCode::SataSsdHeuristics};
        }
    } 
    else if (ssdScore < 0.0f) {
        device.nature = DeviceNature::HDD;
        device.inference = {DeviceNature::HDD, std::min(0.95f, std::abs(ssdScore) + 0.4f), InferenceReasonCode::RotationalSeekPenalty};
    } 
    else {
        device.nature = DeviceNature::SATA_SSD;
        device.inference = {DeviceNature::SATA_SSD, 0.40f, InferenceReasonCode::SataSsdFallback};
    }
}

void StorageClassifier::synthesizeTopology(VolumeTopology& topology) {
    topology.hasSSD = false;
    topology.hasHDD = false;
    topology.hasNVMe = false;
    topology.hasUSB = false;
    if (topology.isNetworkStorage) {
        topology.finalNature = DeviceNature::NetworkStorage;
        topology.finalInference = {DeviceNature::NetworkStorage, 0.95f, InferenceReasonCode::NetworkShareDetected};
        return;
    }
    for (auto it = topology.devices.begin(); it != topology.devices.end(); ++it) {
        PhysicalDevice& dev = it.value();
        classifySingleDevice(dev);
        if (dev.busType == StorageBusType::USB) topology.hasUSB = true;
        if (dev.nature == DeviceNature::HDD) topology.hasHDD = true;
        if (dev.nature == DeviceNature::SATA_SSD || dev.nature == DeviceNature::USB_SSD || dev.nature == DeviceNature::NVME_SSD) {
            topology.hasSSD = true;
        }
        if (dev.nature == DeviceNature::NVME_SSD) topology.hasNVMe = true;
        if (dev.busType == StorageBusType::RAID) topology.isRaid = true;
    }
    if (topology.extents.size() > 1 || topology.devices.size() > 1) {
        topology.isSpanned = true;
    }
    if (topology.isSpanned) {
        if (topology.hasSSD && topology.hasHDD) {
            topology.isHybrid = true;
            topology.finalNature = DeviceNature::Hybrid;
            topology.finalInference = {DeviceNature::Hybrid, 0.90f, InferenceReasonCode::HybridDriveTiering};
        } else if (topology.isStorageSpaces) {
            topology.finalNature = topology.hasSSD ? DeviceNature::SATA_SSD : DeviceNature::HDD;
            topology.finalInference = {topology.finalNature, 0.85f, InferenceReasonCode::StorageSpacesSpanned};
        } else {
            topology.finalNature = topology.hasSSD ? DeviceNature::SATA_SSD : DeviceNature::HDD;
            topology.finalInference = {topology.finalNature, 0.80f, InferenceReasonCode::StorageSpacesSpanned};
        }
        return;
    }
    if (topology.isRaid) {
        topology.finalNature = DeviceNature::RAID;
        topology.finalInference = {DeviceNature::RAID, 0.95f, InferenceReasonCode::HardwareRaidDetected};
        return;
    }
    if (!topology.devices.isEmpty()) {
        const auto& mainDevice = topology.devices.constBegin().value();
        topology.finalNature = mainDevice.nature;
        topology.finalInference = mainDevice.inference;
        topology.finalInference.reasonCode = InferenceReasonCode::SingleDeviceMapping;
        return;
    }
    topology.finalNature = DeviceNature::Unknown;
    topology.finalInference = {DeviceNature::Unknown, 0.10f, InferenceReasonCode::InsufficientEvidence};
}