#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
struct T3DVec3 { float v[3]; };
struct T3DMat4 { float m[4][4]{}; };
using T3DMat4FP = T3DMat4;
struct T3DVertPacked { uint32_t rgbaA = 0, rgbaB = 0; };
struct T3DViewport {};
struct T3DInitParams {};
using T3DDrawFlags = uint32_t;
constexpr unsigned T3D_FLAG_SHADED=1, T3D_FLAG_DEPTH=2, T3D_FLAG_CULL_BACK=4, T3D_FLAG_NO_LIGHT=8;
#define T3D_DEG_TO_RAD(value) ((value)*0.017453292519943295f)
namespace deferred_rsp {
struct Pending { const T3DVertPacked* vertices; const T3DMat4FP* matrix; };
struct Consumed { uint32_t color; float x, y, z; };
inline std::vector<Pending> pending;
inline std::vector<Consumed> consumed;
inline const T3DVertPacked* vertices = nullptr;
inline const T3DMat4FP* matrix = nullptr;
inline void Drain() {
 for(const auto& item:pending) consumed.push_back({item.vertices[0].rgbaA,item.matrix->m[3][0],item.matrix->m[3][1],item.matrix->m[3][2]});
 pending.clear();
}
}
inline void t3d_init(T3DInitParams) {}
inline void t3d_destroy() {}
inline T3DViewport t3d_viewport_create() { return {}; }
inline void t3d_viewport_set_projection(T3DViewport*,float,float,float) {}
inline void t3d_viewport_look_at(T3DViewport*,const T3DVec3*,const T3DVec3*,const T3DVec3*) {}
inline void t3d_viewport_set_perspective(T3DViewport*,float,float,float,float) {}
inline void t3d_viewport_set_view_matrix(T3DViewport*,const T3DMat4*) {}
inline void t3d_viewport_attach(T3DViewport*) {}
inline void t3d_frame_start() {}
inline void t3d_mat4_rotate(T3DMat4*,const T3DVec3*,float) {}
inline void t3d_mat4_mul(T3DMat4*,const T3DMat4*,const T3DMat4*) {}
inline void t3d_mat4_look_at(T3DMat4*,const T3DVec3*,const T3DVec3*,const T3DVec3*) {}
inline void t3d_mat4_from_srt(T3DMat4* matrix,const float*,const float*,const float* position) { for(int i=0;i<3;i++)matrix->m[3][i]=position[i]; }
inline void t3d_mat4_to_fixed(T3DMat4FP* output,const T3DMat4* input) { *output=*input; }
inline void t3d_light_set_ambient(const uint8_t*) {}
inline void t3d_light_set_directional(int,const uint8_t*,const T3DVec3*) {}
inline void t3d_light_set_count(int) {}
inline void t3d_state_set_drawflags(T3DDrawFlags) {}
inline void t3d_matrix_push(const T3DMat4FP* matrix) { deferred_rsp::matrix=matrix; }
inline void t3d_matrix_pop(int) {}
inline void t3d_vert_load(const T3DVertPacked* vertices,unsigned,unsigned) { deferred_rsp::vertices=vertices; }
inline void t3d_tri_draw(unsigned,unsigned,unsigned) { deferred_rsp::pending.push_back({deferred_rsp::vertices,deferred_rsp::matrix}); }
// A queued triangle sync does not imply CPU ownership of its DMA inputs.
inline void t3d_tri_sync() {}
