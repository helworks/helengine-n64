#include "Nintendo64Renderer.hpp"

#include <libdragon.h>

namespace helengine::n64 {

bool Nintendo64Renderer::Initialize() {
    if (Initialized) {
        return true;
    }

    T3DInitParams params{};
    t3d_init(params);

    ModelMatrix = static_cast<T3DMat4FP*>(malloc_uncached(sizeof(T3DMat4FP)));
    if (ModelMatrix == nullptr) {
        debugf("n64:error model-matrix-allocation-failed\n");
        t3d_destroy();
        return false;
    }

    Viewport = t3d_viewport_create();
    t3d_viewport_set_projection(&Viewport, T3D_DEG_TO_RAD(60.0f), 1.0f, 100.0f);
    const T3DVec3 cameraPosition = {{0.0f, 0.0f, -18.0f}};
    const T3DVec3 cameraTarget = {{0.0f, 0.0f, 0.0f}};
    const T3DVec3 cameraUp = {{0.0f, 1.0f, 0.0f}};
    t3d_viewport_look_at(&Viewport, &cameraPosition, &cameraTarget, &cameraUp);

    if (!Cube.Initialize()) {
        free_uncached(ModelMatrix);
        ModelMatrix = nullptr;
        t3d_destroy();
        return false;
    }

    Initialized = true;
    return true;
}

void Nintendo64Renderer::BeginScene() {
    if (!Initialized) {
        return;
    }

    t3d_frame_start();
    t3d_viewport_attach(&Viewport);
}

void Nintendo64Renderer::Prepare(float angleX, float angleY) {
    if (!Initialized || ModelMatrix == nullptr) {
        return;
    }

    const T3DVec3 axisX = {{1.0f, 0.0f, 0.0f}};
    const T3DVec3 axisY = {{0.0f, 1.0f, 0.0f}};
    T3DMat4 rotationX;
    T3DMat4 rotationY;
    T3DMat4 model;
    t3d_mat4_rotate(&rotationX, &axisX, angleX);
    t3d_mat4_rotate(&rotationY, &axisY, angleY);
    t3d_mat4_mul(&model, &rotationY, &rotationX);
    t3d_mat4_to_fixed(ModelMatrix, &model);
}

void Nintendo64Renderer::DrawCube() {
    if (!Initialized || ModelMatrix == nullptr) {
        return;
    }

    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    rdpq_mode_antialias(AA_NONE);
    rdpq_mode_zbuf(true, true);
    rdpq_mode_blender(0);
    rdpq_mode_fog(0);

    // The NO_LIGHT flag is intentional: vertex RGBA is the complete color
    // source, so the cube is unlit and does not depend on ambient lights.
    t3d_light_set_count(0);
    t3d_state_set_drawflags(
        static_cast<T3DDrawFlags>(T3D_FLAG_SHADED |
                                   T3D_FLAG_DEPTH |
                                   T3D_FLAG_NO_LIGHT |
                                   T3D_FLAG_CULL_BACK));

    t3d_matrix_push(ModelMatrix);
    Cube.Draw();
    t3d_matrix_pop(1);
    t3d_tri_sync();
}

void Nintendo64Renderer::Shutdown() {
    if (!Initialized && ModelMatrix == nullptr) {
        return;
    }

    Cube.Shutdown();
    if (ModelMatrix != nullptr) {
        free_uncached(ModelMatrix);
        ModelMatrix = nullptr;
    }
    t3d_destroy();
    Initialized = false;
}

} // namespace helengine::n64
