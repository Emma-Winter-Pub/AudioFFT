#include "StorageWindowsNetworkHelper.h"

#include <windows.h>
#include <winnetwk.h>
#include <vector>

#pragma comment(lib, "mpr.lib")

void StorageWindowsNetworkHelper::resolveNetworkStorage(VolumeTopology& topology) {
    topology.domain = StorageDomain::NetworkFileSystem;
    topology.isNetworkStorage = true;
    topology.finalNature = DeviceNature::NetworkStorage;
    QString filePath = topology.originalFilePath;
    filePath.replace("/", "\\");
    QString uncPath = filePath;
    if (filePath.length() >= 2 && filePath[1] == ':') {
        QString driveLetter = filePath.left(2);
        DWORD bufSize = 1024;
        std::vector<WCHAR> buffer(bufSize, 0);
        DWORD result = WNetGetConnectionW(
            driveLetter.toStdWString().c_str(), 
            buffer.data(), 
            &bufSize
        );
        if (result == NO_ERROR) {
            QString remoteName = QString::fromWCharArray(buffer.data());
            uncPath = remoteName + filePath.mid(2);
        }
    }
    if (uncPath.startsWith("\\\\")) {
        topology.networkInfo.uncPath = uncPath;
        QString withoutPrefix = uncPath.mid(2);
        int firstSlash = withoutPrefix.indexOf('\\');
        if (firstSlash != -1) {
            topology.networkInfo.server = withoutPrefix.left(firstSlash);
            int secondSlash = withoutPrefix.indexOf('\\', firstSlash + 1);
            if (secondSlash != -1) {
                topology.networkInfo.share = withoutPrefix.mid(firstSlash + 1, secondSlash - firstSlash - 1);
            } else {
                topology.networkInfo.share = withoutPrefix.mid(firstSlash + 1);
            }
        }
        topology.transport = StorageTransport::SMB;
        topology.networkInfo.protocol = "SMB/CIFS";
        topology.finalInference = { 
            DeviceNature::NetworkStorage, 
            0.95f, 
            InferenceReasonCode::NetworkShareDetected
        };
    } else {
        topology.finalInference = { 
            DeviceNature::NetworkStorage, 
            0.50f, 
            InferenceReasonCode::InsufficientEvidence
        };
    }
}