# N64 v0.2 — one authored Helengine scene

**Status:** Planning draft; implementation has not started.

**Goal:** Build a normal Helengine project for platform `n64`, boot its generated runtime, load one authored scene, and render its camera, mesh, transform, and unlit material through Tiny3D.

**Implementation model:** Luna xhigh, one sequential agent. Escalate only for an evidenced unresolved problem. No automatic reviewers, extra agents, or special independent review.

## Baseline

The working native host is on branch `n64-v01` in `C:\dev\helworks\helengine-n64-impl`, inspected at commit `415035a8d18b9320435203ddbd6292c5f3ab3356`. The main checkout still contains the documentation-only initial commit. Start from the implemented branch, not main. Recheck revisions before execution.

The user reports a multicolored cube in the ROM. This establishes user-observed rendering, not a ten-minute stability test or 4 MiB hardware certification. Preserve the standalone cube as a selectable diagnostic build throughout integration.

## Scope

- One authored perspective camera, one static mesh entity, one opaque unlit platform material.
- Read authored world transform, camera transform/projection, and material base color. Do not retain the hardcoded rotating cube as the scene renderer.
- Generated Helengine runtime owns scene loading and frame extraction. Native N64 code owns video, mesh conversion, Tiny3D calls, and asynchronous resource lifetimes.
- Reuse normal cooked scene/model/material serialization and the generic build manifest. Package those files in DragonFS and expose `rom:/` through the content-stream abstraction.
- Retain 320×240, 16-bit color/depth, double buffering, explicit no-light state, and the conservative frame-completion wait.
- Target 4 MiB, with no expansion requirement. Measure the generated runtime footprint before expanding integration.
- No textures, animation, scripts, physics, audio playback, controller implementation, UI, multiple cameras, or scene switching. Required empty backend adapters may support zero work; unsupported requests must fail explicitly.

A focused initial geometry limit is one unskinned triangle-list submesh with at most 64 distinct packed vertices and 128 triangles. The cube fits comfortably. These are temporary v0.2 capability limits, not N64 hardware claims. Validate indices, finite coordinates, and the Tiny3D signed-16-bit position range; reject excess instead of dropping geometry or truncating numbers.

## Architecture

1. The existing platform discovery service loads an N64 descriptor and builder assembly.
2. The existing editor build graph generates C++, resolves scene dependencies, and cooks content.
3. The N64 builder validates the supported subset, stages generated sources and cooked payloads, writes a startup manifest, and builds a DragonFS-bearing ROM using the existing image/toolchain.
4. An N64 runtime host initializes Core with platform adapters, mounts DragonFS, installs the scene catalog, and loads the selected scene through the existing scene/content managers.
5. An N64 RenderManager3D adapter consumes normal camera/drawable submissions. It creates persistent Tiny3D mesh/matrix resources and forwards draws to the native renderer.

Keep the native debug host and authored-runtime host as explicit modes. Missing generated code or scene content in authored mode is an error, never a fallback to the cube.

## Important findings from source inspection

- Generic discovery: `helengine/engine/helengine.platforms/DevelopmentPlatformProvider.cs`, `PlatformInstallationStore.cs`, and `PlatformDescriptorStore.cs` use platform manifests/descriptors. No central hardcoded N64 editor registry is needed initially.
- `IPlatformAssetBuilder` exposes Descriptor, Definition, CookMaterial, and BuildAsync. BuildAsync receives the resolved manifest and GeneratedCoreCppRootPath.
- `EditorGeneratedCoreRegenerationService.cs` drives the actual C++ generator from builder-provided codegen metadata.
- `EditorPlatformBuildGraphRunner.cs` currently creates cook profiles declaring **LittleEndian**. N64 CPU endianness does not authorize changing shared serialized formats globally.
- DS startup demonstrates Core initialization, content source, scene catalog, render managers, and scene loading. Its mature services and source-rewriting stager are not templates to copy wholesale.
- GameCube MaterialCooker already emits `PlatformMaterialAsset` through `AssetSerializer`, including Lit, UseVertexColor, and BaseColor fields. Reuse that contract for N64's restricted unlit schema.
- `RenderManager3D` provides BuildModelFromRaw, BuildModelFromCooked, BuildMaterialFromCooked, release hooks, capabilities, and Draw. Verify generated signatures rather than guessing C++ naming.
- The actual existing N64 renderer is `Nintendo64Renderer`, not the earlier draft name Nintendo64RasterRenderer. Actual clock files are `FrameClock.*`.

## Acceptance

1. A normal project build for `n64` produces a fresh standalone ROM with the selected authored scene.
2. Logs identify runtime initialization, startup scene ID, loaded mesh/material IDs, first submitted authored draw, and first completed frame.
3. Change cube translation and base color in the authored project, rebuild, and observe both changes. No native scene constants are edited.
4. Changing the camera also changes the result; the current hardcoded camera is not used.
5. Missing assets and unsupported scene/material features fail clearly; failed builds do not publish a stale successful ROM.
6. Generated sources are never postprocessed to repair generator errors. Necessary fixes occur at the generator/source seam, with a focused test.
7. Base-memory evidence includes ELF loadable sections, heap/stack headroom, and runtime allocations. Emulator and hardware results remain distinct.

## Cost controls and gates

Gate A: reuse the installed SDK image and verify minimal generated runtime compile/boot before implementing all cooking/render adapters. Do not rebuild the compiler unless necessary.

Gate B: load the authored scene and enumerate expected objects before debugging rasterization. Gate C: draw and validate editor-to-ROM changes.

If the generated runtime cannot fit 4 MiB or needs broad generator/runtime surgery, document the concrete cause and return for a revised plan. Do not silently require 8 MiB, replace Core with a custom scene interpreter, or escalate the model. Later tasks remain in scope only if the gate is feasible.
