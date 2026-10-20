#pragma once
#include <cstdlib>
#include "t3d/t3d.h"
inline void* malloc_uncached(std::size_t size) { return std::malloc(size); }
inline void free_uncached(void* pointer) { std::free(pointer); }
inline void debugf(const char*,...) {}
inline void rspq_wait() { deferred_rsp::Drain(); }
constexpr int RDPQ_COMBINER_SHADE=0,AA_NONE=0;
enum tex_format_t {FMT_RGBA16,FMT_CI4,FMT_CI8};
enum rdpq_tlut_t {TLUT_NONE,TLUT_RGBA16,TLUT_IA16};
struct surface_t {uint16_t width=0,height=0;tex_format_t format=FMT_RGBA16;bool fits=true;};
struct rdpq_texparms_t {};
constexpr int TILE0=0,RDPQ_COMBINER_TEX_SHADE=1,FILTER_POINT=0;
namespace texture_state {inline int combiner=-1,uploads=0,tlut=-1,palettes=0;inline bool perspective=false;inline const surface_t* surface=nullptr;}
inline void rdpq_mode_combiner(int value) {texture_state::combiner=value;}
inline void rdpq_mode_tlut(rdpq_tlut_t value) {texture_state::tlut=value;}
inline void rdpq_mode_filter(int) {}
inline void rdpq_mode_alphacompare(int) {}
inline void rdpq_mode_persp(bool value) {texture_state::perspective=value;}
inline bool rdpq_tex_can_upload(const surface_t* value) {return value!=nullptr&&value->fits;}
inline int rdpq_tex_upload(int,const surface_t* surface,const rdpq_texparms_t*) {texture_state::uploads++;texture_state::surface=surface;return 0;}
inline void rdpq_mode_antialias(int) {}
inline void rdpq_mode_zbuf(bool,bool) {}
inline void rdpq_mode_blender(int) {}
inline void rdpq_mode_fog(int) {}
