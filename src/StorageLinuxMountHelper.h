#pragma once

#include "StorageLinuxTypes.h"

class LinuxMountHelper {
public:
    static bool getMountInfoForPath(const QString& targetPath, MountInfo& outInfo);
private:
    static QString unescapeMountInfo(const QString& str);
};

class NetworkResolver : public ITopologyNodeResolver {
public:
    ResolveResult resolve(const ResolvedNode& node, ResolveContext& context) override;
};