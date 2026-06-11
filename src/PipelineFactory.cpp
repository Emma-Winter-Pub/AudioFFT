#include "PipelineFactory.h"
#include "IParallelPipeline.h"

PipelineFactory& PipelineFactory::instance(){
    static PipelineFactory inst;
    return inst;
}

void PipelineFactory::registerPipeline(const std::string& codecName, Creator creator){
    if (codecName.empty() || !creator) {
        return;
    }
    m_registry[codecName] = std::move(creator);
}

std::unique_ptr<IParallelPipeline> PipelineFactory::create(const std::string& codecName) const{
    auto it = m_registry.find(codecName);
    if (it != m_registry.end()) {
        return it->second();
    }
    return nullptr;
}