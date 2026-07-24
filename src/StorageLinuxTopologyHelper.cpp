#include "StorageLinuxTopologyHelper.h"
#include "StorageLinuxMountHelper.h"
#include "StorageLinuxAttributeHelper.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

std::vector<std::unique_ptr<ITopologyNodeResolver>> LinuxTopologyHelper::createResolverPipeline() {
    std::vector<std::unique_ptr<ITopologyNodeResolver>> resolvers;
    resolvers.push_back(std::make_unique<NetworkResolver>());
    resolvers.push_back(std::make_unique<BtrfsResolver>());
    resolvers.push_back(std::make_unique<ZfsResolver>());
    resolvers.push_back(std::make_unique<LoopResolver>());
    resolvers.push_back(std::make_unique<DmResolver>());
    resolvers.push_back(std::make_unique<MdResolver>());
    resolvers.push_back(std::make_unique<PartitionResolver>());
    resolvers.push_back(std::make_unique<PhysicalResolver>());
    return resolvers;
}

ResolveResult BtrfsResolver::resolve(const ResolvedNode& node, ResolveContext& context) {
    if (context.facts.mountInfo.fsType.toLower() != "btrfs" || context.state.btrfsResolved) {
        return ResolveResult::NotHandled;
    }
    QDir btrfsDir("/sys/fs/btrfs");
    if (!btrfsDir.exists()) return ResolveResult::NotHandled;
    QStringList uuids = btrfsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& uuid : uuids) {
        QDir devDir(QString("/sys/fs/btrfs/%1/devices").arg(uuid));
        if (devDir.exists(node.deviceName)) {
            context.state.btrfsResolved = true;
            QStringList poolDevs = devDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            if (poolDevs.size() > 1) {
                context.facts.isSpanned = true;
            }
            for (const QString& poolDev : poolDevs) {
                if (poolDev != node.deviceName) {
                    context.state.nodeQueue.enqueue(ResolvedNode::fromDevicePath("/dev/" + poolDev));
                }
            }
            return ResolveResult::NotHandled;
        }
    }
    return ResolveResult::NotHandled;
}

ResolveResult ZfsResolver::resolve(const ResolvedNode&, ResolveContext&) {
    return ResolveResult::NotHandled;
}

ResolveResult LoopResolver::resolve(const ResolvedNode& node, ResolveContext& context) {
    if (!node.deviceName.startsWith("loop")) return ResolveResult::NotHandled;
    QString backingFile = LinuxAttributeHelper::readSysfsString(node.sysfsPath + "/loop/backing_file");
    if (!backingFile.isEmpty()) {
        MountInfo info;
        if (LinuxMountHelper::getMountInfoForPath(backingFile, info)) {
            context.state.nodeQueue.enqueue(ResolvedNode::fromDevicePath(info.mountSource));
            return ResolveResult::Continue;
        }
    }
    return ResolveResult::NotHandled;
}

ResolveResult DmResolver::resolve(const ResolvedNode& node, ResolveContext& context) {
    if (!QFile::exists(node.sysfsPath + "/dm")) return ResolveResult::NotHandled;
    QString dmUuid = LinuxAttributeHelper::readSysfsString(node.sysfsPath + "/dm/uuid");
    if (dmUuid.startsWith("CRYPT-", Qt::CaseInsensitive)) {
        context.facts.isEncrypted = true;
    }
    QDir slavesDir(node.sysfsPath + "/slaves");
    QStringList slaves = slavesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (!slaves.isEmpty()) {
        for (const QString& slave : slaves) {
            context.state.nodeQueue.enqueue(ResolvedNode::fromDevicePath("/dev/" + slave));
        }
        return ResolveResult::Continue;
    }
    return ResolveResult::NotHandled;
}

ResolveResult MdResolver::resolve(const ResolvedNode& node, ResolveContext& context) {
    if (!QFile::exists(node.sysfsPath + "/md")) return ResolveResult::NotHandled;
    context.facts.isRaid = true;
    QDir slavesDir(node.sysfsPath + "/slaves");
    QStringList slaves = slavesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (!slaves.isEmpty()) {
        for (const QString& slave : slaves) {
            context.state.nodeQueue.enqueue(ResolvedNode::fromDevicePath("/dev/" + slave));
        }
        return ResolveResult::Continue;
    }
    return ResolveResult::NotHandled;
}

ResolveResult PartitionResolver::resolve(const ResolvedNode& node, ResolveContext& context) {
    if (!QFile::exists(node.sysfsPath + "/partition")) return ResolveResult::NotHandled;
    QString realPath = QFileInfo(node.sysfsPath).canonicalFilePath();
    QString parentDevName = QFileInfo(realPath).absoluteDir().dirName();
    if (!parentDevName.isEmpty()) {
        context.state.nodeQueue.enqueue(ResolvedNode::fromDevicePath("/dev/" + parentDevName));
        return ResolveResult::Continue;
    }
    return ResolveResult::NotHandled;
}

ResolveResult PhysicalResolver::resolve(const ResolvedNode& node, ResolveContext&) {
    if (QFile::exists(node.sysfsPath)) {
        return ResolveResult::Finished;
    }
    return ResolveResult::NotHandled;
}