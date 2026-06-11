#pragma once

#include <map>
#include <string>
#include <functional>
#include <memory>

class IParallelPipeline;

class PipelineFactory {
public:
    using Creator = std::function<std::unique_ptr<IParallelPipeline>()>;
    static PipelineFactory& instance();
    void registerPipeline(const std::string& codecName, Creator creator);
    std::unique_ptr<IParallelPipeline> create(const std::string& codecName) const;

private:
    PipelineFactory() = default;
    PipelineFactory(const PipelineFactory&) = delete;
    PipelineFactory& operator=(const PipelineFactory&) = delete;
    std::map<std::string, Creator> m_registry;
};