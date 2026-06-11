#include "BatchStreamBuffer.h"

#include <algorithm>
#include <cstring>

BatchStreamBuffer::BatchStreamBuffer(size_t capacity)
    : m_capacity(capacity)
    , m_head(0)
    , m_tail(0)
    , m_count(0)
    , m_eof(false)
    , m_aborted(false)
{
    m_buffer.resize(m_capacity);
}

void BatchStreamBuffer::abort() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_aborted = true;
    m_condNotEmpty.notify_all();
    m_condNotFull.notify_all();
}

bool BatchStreamBuffer::write(const uint8_t* data, size_t len) {
    size_t bytesWritten = 0;
    const uint8_t* src = data;
    while (bytesWritten < len) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condNotFull.wait(lock, [this]() {
            return m_count < m_capacity || m_aborted;
        });
        if (m_aborted) return false;
        size_t spaceAvailable = m_capacity - m_count;
        size_t chunk = std::min(len - bytesWritten, spaceAvailable);
        size_t contiguousSpace = m_capacity - m_head;
        if (chunk <= contiguousSpace) {
            std::memcpy(&m_buffer[m_head], src, chunk);
            m_head += chunk;
            if (m_head == m_capacity) m_head = 0;
        } else {
            std::memcpy(&m_buffer[m_head], src, contiguousSpace);
            size_t remaining = chunk - contiguousSpace;
            std::memcpy(&m_buffer[0], src + contiguousSpace, remaining);
            m_head = remaining;
        }
        m_count += chunk;
        src += chunk;
        bytesWritten += chunk;
        m_condNotEmpty.notify_one();
    }
    return true;
}

void BatchStreamBuffer::setEof() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_eof = true;
    m_condNotEmpty.notify_all();
}

int BatchStreamBuffer::read(uint8_t* buf, int len) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condNotEmpty.wait(lock, [this]() {
        return m_count > 0 || m_eof || m_aborted;
    });
    if (m_aborted) return -1;
    if (m_count == 0 && m_eof) return 0;
    size_t requested = static_cast<size_t>(len);
    size_t toRead = std::min(requested, m_count);
    size_t contiguousData = m_capacity - m_tail;
    if (toRead <= contiguousData) {
        std::memcpy(buf, &m_buffer[m_tail], toRead);
        m_tail += toRead;
        if (m_tail == m_capacity) m_tail = 0;
    } else {
        std::memcpy(buf, &m_buffer[m_tail], contiguousData);
        size_t remaining = toRead - contiguousData;
        std::memcpy(buf + contiguousData, &m_buffer[0], remaining);
        m_tail = remaining;
    }
    m_count -= toRead;
    m_condNotFull.notify_one();
    return static_cast<int>(toRead);
}

size_t BatchStreamBuffer::bytesAvailable() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_count;
}

size_t BatchStreamBuffer::capacity() const {return m_capacity;}