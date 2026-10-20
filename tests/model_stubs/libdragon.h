#pragma once
#include <cstdlib>
inline void* malloc_uncached(std::size_t size) { return std::malloc(size); }
inline void free_uncached(void* value) { std::free(value); }
