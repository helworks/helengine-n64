#pragma once

// Host-test stub for Tiny3D. It declares only the packed vertex layout that
// Nintendo64VertexPacking touches; the struct below mirrors the real
// T3DVertPacked in .deps/tiny3d/src/t3d/t3d.h field for field, including the
// 8-byte alignment, so the host tests observe the same slot offsets the N64
// build does.

#include <cstdint>

// Internal vertex format, interleaves two vertices
typedef struct {
  /* 0x00 */ int16_t posA[3]; // s16 (used in the ucode as the int. part of a s16.16)
  /* 0x06 */ uint16_t normA;  // 5,6,5 packed normal
  /* 0x08 */ int16_t posB[3]; // s16 (used in the ucode as the int. part of a s16.16)
  /* 0x0E */ uint16_t normB;  // 5,6,5 packed normal
  /* 0x10 */ uint32_t rgbaA; // RGBA8 color
  /* 0x14 */ uint32_t rgbaB; // RGBA8 color
  /* 0x18 */ int16_t stA[2]; // UV fixed point 10.5 (pixel coords)
  /* 0x1C */ int16_t stB[2]; // UV fixed point 10.5 (pixel coords)
} __attribute__((aligned(8))) T3DVertPacked;

static_assert(sizeof(T3DVertPacked) == 0x20, "T3DVertPacked has wrong size");
