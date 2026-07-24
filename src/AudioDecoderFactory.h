#pragma once

#include <map>
#include <string>
#include <functional>
#include <memory>

class XAudioDecoder;

class AudioDecoderFactory {
public:
    using Creator = std::function<std::unique_ptr<XAudioDecoder>()>;
    static AudioDecoderFactory& instance();
    void registerPipeline(const std::string& codecName, Creator creator);
    std::unique_ptr<XAudioDecoder> create(const std::string& codecName) const;

private:
    AudioDecoderFactory() = default;
    AudioDecoderFactory(const AudioDecoderFactory&) = delete;
    AudioDecoderFactory& operator=(const AudioDecoderFactory&) = delete;
    std::map<std::string, Creator> m_registry;
};