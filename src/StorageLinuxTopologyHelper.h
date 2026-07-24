#pragma once

#include "StorageLinuxTypes.h"

#include <vector>
#include <memory>

class LinuxTopologyHelper {
public:
    static std::vector<std::unique_ptr<ITopologyNodeResolver>> createResolverPipeline();
};

class BtrfsResolver : public ITopologyNodeResolver {
public:
    ResolveResult resolve(const ResolvedNode& node, ResolveContext& context) override;
};

class ZfsResolver : public ITopologyNodeResolver {
public:
    ResolveResult resolve(const ResolvedNode& node, ResolveContext& context) override;
};

class LoopResolver : public ITopologyNodeResolver {
public:
    ResolveResult resolve(const ResolvedNode& node, ResolveContext& context) override;
};

class DmResolver : public ITopologyNodeResolver {
public:
    ResolveResult resolve(const ResolvedNode& node, ResolveContext& context) override;
};

class MdResolver : public ITopologyNodeResolver {
public:
    ResolveResult resolve(const ResolvedNode& node, ResolveContext& context) override;
};

class PartitionResolver : public ITopologyNodeResolver {
public:
    ResolveResult resolve(const ResolvedNode& node, ResolveContext& context) override;
};

class PhysicalResolver : public ITopologyNodeResolver {
public:
    ResolveResult resolve(const ResolvedNode& node, ResolveContext& context) override;
};