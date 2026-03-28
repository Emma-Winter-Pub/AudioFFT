#include "PlayerRamProvider.h"

#include <algorithm>
#include <cstring> 

PlayerRamProvider::PlayerRamProvider(const PcmDataVariant& pcmData, int sampleRate)
    : m_pcmData(pcmData), m_sampleRate(sampleRate) 
{
    bool isDouble = std::holds_alternative<PcmData64>(m_pcmData);
    AVSampleFormat fmt = isDouble ? AV_SAMPLE_FMT_DBL : AV_SAMPLE_FMT_FLT;
    m_targetRate = m_sampleRate;
    m_resampler.init(m_sampleRate, 1, fmt, m_sampleRate);
}

void PlayerRamProvider::configureResampler(int targetRate) {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool isDouble = std::holds_alternative<PcmData64>(m_pcmData);
    AVSampleFormat fmt = isDouble ? AV_SAMPLE_FMT_DBL : AV_SAMPLE_FMT_FLT;
    m_targetRate = targetRate;
    m_resampler.init(m_sampleRate, 1, fmt, targetRate);
    m_resampledCache.clear();
    m_cachePos = 0;
}

void PlayerRamProvider::fillCache() {
    const size_t chunkSize = 4096; 
    auto processChunk = [&](auto&& vec) {
        if (m_currentPos >= vec.size()) return;
        size_t toRead = std::min(chunkSize, vec.size() - m_currentPos);
        std::vector<float> converted = m_resampler.process(&vec[m_currentPos], (int)toRead);
        m_resampledCache.insert(m_resampledCache.end(), converted.begin(), converted.end());
        m_currentPos += toRead;
    };
    std::visit(processChunk, m_pcmData);
}

qint64 PlayerRamProvider::readSamples(float* buffer, qint64 maxFloatCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    qint64 totalReadFloats = 0;
    while (totalReadFloats < maxFloatCount) {
        if (m_cachePos >= m_resampledCache.size()) {
            m_resampledCache.clear();
            m_cachePos = 0;
            fillCache();
            if (m_resampledCache.empty()) break; 
        }
        size_t availableFloats = m_resampledCache.size() - m_cachePos;
        size_t toCopyFloats = std::min((size_t)(maxFloatCount - totalReadFloats), availableFloats);
        std::memcpy(buffer + totalReadFloats, m_resampledCache.data() + m_cachePos, toCopyFloats * sizeof(float));
        m_cachePos += toCopyFloats;
        totalReadFloats += toCopyFloats;
    }
    return totalReadFloats;
}

void PlayerRamProvider::seek(double seconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentPos = static_cast<size_t>(seconds * m_sampleRate);
    std::visit([this](auto&& vec) {
        if (m_currentPos > vec.size()) m_currentPos = vec.size();
    }, m_pcmData);
    m_resampledCache.clear();
    m_cachePos = 0;
    m_resampler.reset();
}

double PlayerRamProvider::currentTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_sampleRate <= 0) return 0.0;
    return static_cast<double>(m_currentPos) / m_sampleRate;
}

bool PlayerRamProvider::atEnd() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool noMoreSrc = false;
    std::visit([this, &noMoreSrc](auto&& vec) {
        noMoreSrc = (m_currentPos >= vec.size());
    }, m_pcmData);
    return noMoreSrc && (m_cachePos >= m_resampledCache.size());
}

double PlayerRamProvider::getLastDecodedTime() const {
    return currentTime(); 
}

double PlayerRamProvider::getInternalBufferDuration() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_targetRate <= 0) return 0.0;
    size_t remainingFloats = m_resampledCache.size() - m_cachePos;
    return static_cast<double>(remainingFloats) / (m_targetRate * 2.0);
}