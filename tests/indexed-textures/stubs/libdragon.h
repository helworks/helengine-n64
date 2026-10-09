#pragma once
#include <cstddef>
#include <cstdint>
enum tex_format_t { FMT_RGBA16=2, FMT_CI4=8, FMT_CI8=9 };
enum rdpq_tlut_t { TLUT_NONE=0, TLUT_RGBA16=2, TLUT_IA16=3 };
struct surface_t { void* buffer=nullptr; uint16_t width=0,height=0,stride=0; tex_format_t format=FMT_RGBA16; };
surface_t surface_alloc(tex_format_t format,uint16_t width,uint16_t height);
void surface_free(surface_t* surface);
void* malloc_uncached_aligned(size_t alignment,size_t size);
void free_uncached(void* pointer);
void rspq_wait();
void rdpq_mode_tlut(rdpq_tlut_t mode);
void rdpq_tex_upload_tlut(uint16_t* palette,int first,int count);
