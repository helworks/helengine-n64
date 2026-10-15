#pragma once
#include <cstdlib>
#include "t3d/t3d.h"
inline void* malloc_uncached(std::size_t size) { return std::malloc(size); }
inline void free_uncached(void* pointer) { std::free(pointer); }
inline void debugf(const char*,...) {}
inline void rspq_wait() { deferred_rsp::Drain(); }
constexpr int RDPQ_COMBINER_SHADE=0,AA_NONE=0;
inline void rdpq_mode_combiner(int) {}
inline void rdpq_mode_antialias(int) {}
inline void rdpq_mode_zbuf(bool,bool) {}
inline void rdpq_mode_blender(int) {}
inline void rdpq_mode_fog(int) {}
