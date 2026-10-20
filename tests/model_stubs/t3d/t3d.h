#pragma once
#include "../../stubs/t3d/t3d.h"
struct T3DVec3 { float v[3]; };
// Normal math belongs to the Tiny3D SDK, outside these UV/storage tests.
inline uint16_t t3d_vert_pack_normal(const T3DVec3*) { return 0x1234; }
