#pragma once

#include <vector>
#include <limits>
#include <new>
#include <cstdlib>

#if defined(_MSC_VER)
    #include <malloc.h>
#endif

template <typename T, size_t Alignment = 64>
struct AlignedAllocator {
    using value_type = T;
    AlignedAllocator() noexcept = default;
    template <class U> 
    AlignedAllocator(const AlignedAllocator<U, Alignment>&) noexcept {}
    [[nodiscard]] T* allocate(std::size_t n) {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_array_new_length();
        size_t bytes = n * sizeof(T);
        void* p = nullptr;
#if defined(_MSC_VER)
        p = _aligned_malloc(bytes, Alignment);
#else
        if (posix_memalign(&p, Alignment, bytes) != 0) {
            p = nullptr;
        }
#endif
        if (!p) throw std::bad_alloc();
        return static_cast<T*>(p);
    }
    void deallocate(T* p, std::size_t) noexcept {
#if defined(_MSC_VER)
        _aligned_free(p);
#else
        free(p);
#endif
    }
    template <class U>
    struct rebind { using other = AlignedAllocator<U, Alignment>; };
    bool operator==(const AlignedAllocator&) const noexcept { return true; }
    bool operator!=(const AlignedAllocator&) const noexcept { return false; }
};