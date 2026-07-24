#include "StorageLinuxBackend.h"
#include "StorageLinuxTypes.h"
#include "StorageLinuxMountHelper.h"
#include "StorageLinuxTopologyHelper.h"
#include "StorageLinuxAttributeHelper.h"

#include <QFileInfo>

VolumeTopology LinuxStorageBackend::analyze(const QString& filePath) {
    ResolveContext ctx;
    ctx.input.originalFilePath = filePath;
    if (!LinuxMountHelper::getMountInfoForPath(filePath, ctx.facts.mountInfo)) {
        VolumeTopology empty;
        empty.originalFilePath = filePath;
        empty.finalNature = DeviceNature::Unknown;
        empty.finalInference = {DeviceNature::Unknown, 0.0f, InferenceReasonCode::InsufficientEvidence};
        return empty;
    }
    auto resolvers = LinuxTopologyHelper::createResolverPipeline();
    QString mntSrc = ctx.facts.mountInfo.mountSource;
    if (mntSrc.startsWith("/dev/")) {
        mntSrc = QFileInfo(mntSrc).canonicalFilePath();
    }
    ctx.state.nodeQueue.enqueue(ResolvedNode::fromDevicePath(mntSrc));
    while (!ctx.state.nodeQueue.isEmpty()) {
        ResolvedNode node = ctx.state.nodeQueue.dequeue();
        if (ctx.state.visitedNodes.contains(node.devicePath)) continue;
        ctx.state.visitedNodes.insert(node.devicePath);
        bool handled = false;
        for (const auto& resolver : resolvers) {
            ResolveResult res = resolver->resolve(node, ctx);
            if (res == ResolveResult::Continue) {
                handled = true;
                break;
            } else if (res == ResolveResult::Finished) {
                if (node.devicePath.startsWith("/dev/") || ctx.facts.isNetwork) {
                    ctx.state.leafNodes.append(node);
                }
                handled = true;
                break;
            } else if (res == ResolveResult::Error) {
                handled = true;
                break;
            }
        }
        if (!handled && node.devicePath.startsWith("/dev/")) {
            ctx.state.leafNodes.append(node);
        }
    }
    VolumeTopology topology;
    topology.originalFilePath = filePath;
    topology.volumePath = ctx.facts.mountInfo.mountPoint;
    topology.filesystem = ctx.facts.mountInfo.fsType;
    if (ctx.facts.isNetwork) {
        topology.isNetworkStorage = true;
        topology.domain = StorageDomain::NetworkFileSystem;
        topology.networkInfo.uncPath = ctx.facts.uncPath;
        topology.networkInfo.server = ctx.facts.server;
        topology.networkInfo.share = ctx.facts.share;
        topology.networkInfo.protocol = ctx.facts.networkProtocol;
        if (ctx.facts.networkProtocol.startsWith("SMB")) topology.transport = StorageTransport::SMB;
        else if (ctx.facts.networkProtocol.startsWith("NFS")) topology.transport = StorageTransport::NFS;
    } else {
        topology.domain = StorageDomain::LocalPhysical;
        LinuxAttributeHelper::resolveAttributes(ctx.state.leafNodes, topology.devices);
    }
    topology.isEncrypted = ctx.facts.isEncrypted;
    topology.isRaid = ctx.facts.isRaid;
    topology.isSpanned = ctx.facts.isSpanned;
    return topology;
}