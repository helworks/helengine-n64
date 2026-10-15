#pragma once
#include <cstdint>

struct surface_t { int marker; };
struct color_t { uint8_t r, g, b, a; };
inline color_t RGBA32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return {r, g, b, a}; }
constexpr int RESOLUTION_320x240 = 1, DEPTH_16_BPP = 1, GAMMA_NONE = 0, FILTERS_RESAMPLE = 1;
constexpr uint16_t ZBUF_MAX = 0xFFFC;
void display_init(int, int, int, int, int);
void display_close();
surface_t* display_get();
surface_t* display_get_zbuf();
void display_show(surface_t*);
void rdpq_init();
void rdpq_close();
void rdpq_attach(surface_t*, surface_t*);
void rdpq_detach_wait();
void rdpq_clear(color_t);
void rdpq_clear_z(uint16_t);
void debugf(const char*, ...);
