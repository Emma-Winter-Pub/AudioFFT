#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <cstddef>
#include <atomic>

class BatchStreamBuffer {
public:
    explicit BatchStreamBuffer(size_t capacity = 128 * 1024 * 1024);
    ~BatchStreamBuffer() = default;
    BatchStreamBuffer(const BatchStreamBuffer&) = delete;
    BatchStreamBuffer& operator=(const BatchStreamBuffer&) = delete;
    bool write(const uint8_t* data, size_t len);
    void setEof();
    int read(uint8_t* buf, int len);
    void abort();
    size_t bytesAvailable() const;
    size_t capacity() const;

private:
    std::vector<uint8_t> m_buffer;
    size_t m_capacity;
    size_t m_head; 
    size_t m_tail; 
    size_t m_count;
    bool m_eof;
    std::atomic<bool> m_aborted;
    mutable std::mutex m_mutex;
    std::condition_variable m_condNotEmpty;
    std::condition_variable m_condNotFull;
};