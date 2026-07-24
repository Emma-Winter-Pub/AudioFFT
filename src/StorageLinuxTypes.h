#pragma once

#include "StorageTypes.h"

#include <QString>
#include <QSet>
#include <QList>
#include <QQueue>

enum class ResolveResult {
    NotHandled,
    Continue,
    Finished,
    Error
};

struct ResolvedNode {
    QString devicePath;
    QString deviceName;
    QString sysfsPath;
    static ResolvedNode fromDevicePath(const QString& path) {
        ResolvedNode n;
        n.devicePath = path;
        n.deviceName = path.mid(path.lastIndexOf('/') + 1);
        n.sysfsPath = "/sys/class/block/" + n.deviceName;
        return n;
    }
};

struct MountInfo {
    QString mountPoint;
    QString fsType;
    QString mountSource;
};

struct ResolveFacts {
    bool isNetwork = false;
    bool isEncrypted = false;
    bool isRaid = false;
    bool isSpanned = false;
    QString networkProtocol;
    QString uncPath;
    QString server;
    QString share;
    MountInfo mountInfo;
};

struct ResolveInput {
    QString originalFilePath;
};

struct ResolveState {
    QSet<QString> visitedNodes;
    QQueue<ResolvedNode> nodeQueue;
    QList<ResolvedNode> leafNodes;
    bool btrfsResolved = false;
};

struct ResolveContext {
    ResolveInput input;
    ResolveState state;
    ResolveFacts facts;
};

class ITopologyNodeResolver {
public:
    virtual ~ITopologyNodeResolver() = default;
    virtual ResolveResult resolve(const ResolvedNode& node, ResolveContext& context) = 0;
};