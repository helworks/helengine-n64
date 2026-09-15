# N64 Authored Scene Implementation Plan

> **For agentic workers:** Use `superpowers:executing-plans`, sequentially with **gpt-5.6-luna at xhigh**. No nested agents or independent-review workflow. This is a staged integration plan: resolve the actual generated ABI at Task 1 before writing adapters dependent on it.

**Goal:** Build and render one camera, mesh, transform, and unlit material authored in a Helengine project.

**Architecture:** Reuse generic platform discovery, generated Core, shared serialization, and build manifests. Add N64 adapters and DragonFS packaging around the existing native video/Tiny3D host.

**Tech stack:** Existing pinned libdragon/Tiny3D Docker image, GNU C++17/o64, existing C# builder contracts, generated C++, DragonFS.

**Spec:** [v0.2 design](../specs/2026-09-15-n64-v02-authored-scene.md).

## Global constraints

Read the spec, original references, applicable AGENTS.md files, and actual implementation. Start from `n64-v01` in `C:\dev\helworks\helengine-n64-impl`; do not implement atop the documentation-only main checkout. Make local checkpoint commits and do not push or merge without instructions. Preserve unrelated changes in every repository.

Use Luna xhigh throughout. Reuse the existing SDK/build images and compiler; no toolchain rebuild absent a demonstrated need. Keep outputs and logs under `C:\dev\helworks\builds\helengine-n64\v02`, never agent-directed AppData Temp output. Avoid broad builds/tests when focused ones establish the checkpoint.

One camera, one unskinned triangle-list submesh, one opaque unlit material; ≤64 packed vertices and ≤128 triangles. Reject unsupported features, not silently ignore them. Preserve 4 MiB, 320×240, 16-bit color/depth, double buffering, NO_LIGHT, and completion-before-reuse. No generated-source rewriting.

The design authorizes planning only; implementation begins when assigned by the user. No new agent is launched by this document.

## Source map — inspect only the relevant seams

Paths beginning `helengine/` are relative to `C:\dev\helworks`. N64 paths are relative to its implementation checkout.

| Concern | Existing source |
|---|---|
| Discovery/descriptor schema | `helengine/engine/helengine.platforms/{DevelopmentPlatformProvider,PlatformInstallationStore,PlatformInstallationResolver,PlatformDescriptorStore}.cs` |
| Builder contract | `helengine/engine/helengine.baseplatform/Builders/IPlatformAssetBuilder.cs` |
| Request/content manifests | `helengine/engine/helengine.baseplatform/{Requests/PlatformBuildRequest,Manifest/PlatformBuildManifest}.cs` |
| Existing builder/profile | `helengine-ds/builder/{NintendoDsPlatformAssetBuilder,NintendoDsPlatformDefinitionFactory}.cs` |
| Fixed-function material cooking | `helengine-gc/builder/GameCubeMaterialCooker.cs` |
| Generate C++ | `helengine/engine/helengine.editor/managers/project/EditorGeneratedCoreRegenerationService.cs` |
| Build request and serialization declaration | `helengine/engine/helengine.editor/managers/project/EditorPlatformBuildGraphRunner.cs` |
| Native manifest generation | `helengine/engine/helengine.editor/managers/project/EditorRuntimeNativeManifestWriter.cs` |
| Generated boot-scene policy | `helengine/engine/helengine.editor/managers/project/EditorGeneratedBootScenePreparationService.cs` |
| Runtime startup reference | `helengine-ds/src/platform/ds/NintendoDsBootHost.cpp` |
| Runtime renderer contracts | `helengine/engine/helengine.core/managers/rendering/{RenderManager3D,RenderFrameExtractionService,RenderFrameDrawableSubmission}.cs` |
| Content/scene adapter patterns | `helengine-ds/src/platform/ds/{NintendoDsContentStreamSource,NintendoDsPackagedAssetLoader,NintendoDsRenderQueueSnapshotVisitor}.*` |
| Current native host | `src/platform/n64/{Nintendo64BootHost,Nintendo64Video,Nintendo64Renderer,Nintendo64CubeMesh,FrameClock}.*` |

Do not copy DS source-rewriting stager logic or GameCube's historic hardcoded content paths. Resolve current actual project roots/framework versions at execution; do not create references to similarly named duplicate repositories by assumption.

## Task 1 — Platform metadata and generated-runtime feasibility

**Create:** `builder/helengine.n64.builder.csproj`, `builder/Program.cs`, `builder/Nintendo64PlatformDefinitionFactory.cs`, `builder/Nintendo64PlatformAssetBuilder.cs`, `builder.tests/helengine.n64.builder.tests.csproj`, `builder.tests/Nintendo64PlatformDefinitionTests.cs`, `scripts/register-development-platform.ps1`, `docs/RuntimeCompatibility.md`.

**Modify:** N64 Makefile/build wrapper for an explicit `HELENGINE_N64_GENERATED_CORE_ROOT` and mode selection. Modify shared generator sources only for a reproducible platform-neutral defect, with tests in their owning repository.

**Interfaces:** Implement the existing IPlatformAssetBuilder contract verbatim. IDs: platform `n64`, graphics profile `n64-unlit`, material schema `n64-unlit-v1`, build profiles `debug` and `release`. Use C++ codegen metadata from the existing definitions; copy only required fields and feature declarations.

- [ ] Record source commits and current SDK image identity. Preserve the existing diagnostic ROM/hash before changes. Create an implementation branch from the completed host; keep planning docs accessible.
- [ ] Build a minimal builder assembly using the same resolved baseplatform/core/files references and appropriate current target framework as sibling plugins. Start with tests that assert stable platform/profile IDs and no advertised lighting/textures/audio support.
- [ ] Implement `--describe` and metadata. Before native packaging exists, BuildAsync must fail explicitly with a not-yet-supported diagnostic, never return success. This intermediate state is a local checkpoint, not the final platform deliverable.
- [ ] Implement development registration against the actual `platforms.json` schema. Parameters: EngineUserSettingsRoot, EngineVersion, N64RepositoryRoot, BuilderAssemblyPath, GeneratedCoreCppRoot. Read existing entries and update only n64; validate actual paths, preserve other platforms, and write atomically. Support an isolated registration root for tests before updating the user's development catalog. Resolve the engine version from the selected engine build, never invent it.
- [ ] Test registration twice for idempotence, preservation of other platform entries, and resolution of the N64 player source root to the implementation checkout. Reuse PlatformInstallationResolver instead of adding a new editor registry.
- [ ] Create a minimal authored fixture under `samples/AuthoredCube` using existing project/scene serializers and a small generator tool `tools/helengine.n64.samplegen`. One camera, one primitive cube model and one platform material; no scripts, physics, textures, audio, or decorative generated splash scene. Before generation, check the normal build graph's generated boot-scene behavior: either use an existing supported disable policy or plan a narrow capability-based change with a regression test. Do not silently strip splash components after cooking.
- [ ] Invoke existing C++ generation through the normal editor/build service for this fixture. Record exact command/profile/features and emitted header/source manifest in RuntimeCompatibility.md. Inspect generated Core, initialization options, RenderManager3D/2D, input, content-stream, scene-manager, and material/model signatures.
- [ ] Compile/link the minimal generated runtime with the N64 target and a boot probe. Only provide required zero-work input/2D adapters; reject calls requesting unsupported work. Instantiate Core, initialize its supported dependencies, and advance an empty frame. This first probe can omit scene packaging, but may not substitute fake Core classes.
- [ ] Inspect target ABI assumptions, filesystem/clock calls, atomics/thread requirements, exception configuration, byte order, and ELF loadable/BSS footprint. Keep default target flags; do not blanket-disable exceptions when shared runtime behavior requires them. Narrow a feature only through supported codegen configuration or a source-level fix.
- [ ] Record actual generated override declarations in RuntimeCompatibility.md before implementing Tasks 2–4. Generated methods are authoritative; do not type guessed signatures from this plan.

**Gate A:** target compile/link and runtime initialization evidence, with plausible 4 MiB headroom. If blocked on broad generator changes or memory, return a concise failing command/symbol/size report and stop dependent implementation. Continue useful tests/docs. Do not silently switch to 8 MiB or a custom scene reader.

**Validation commands:** `dotnet test builder.tests/helengine.n64.builder.tests.csproj --filter FullyQualifiedName~Nintendo64PlatformDefinitionTests`; build the generated boot probe with the existing Docker image. A compiler success is not a runtime-boot success.

**Commit:** `feat(n64): add platform metadata and generated runtime bootstrap`.

## Task 2 — Cook the supported subset and package DragonFS

**Create:** `builder/{Nintendo64MaterialCooker,Nintendo64SceneCompatibilityValidator,Nintendo64ContentStager,Nintendo64StartupManifestWriter,Nintendo64NativeBuildExecutor}.cs`, `builder/INintendo64NativeBuildExecutor.cs`; matching focused tests under builder.tests.

**Modify:** builder BuildAsync, Makefile, Docker build wrapper; existing shared paths only if a failing integration test identifies missing generic support.

**Interfaces:** Use PlatformBuildRequest.Manifest, GeneratedCoreCppRootPath, WorkingRoot, OutputRoot, and cancellation token. Reuse PlatformMaterialAsset and AssetSerializer. INintendo64NativeBuildExecutor exposes one build operation taking generated source root, staged filesystem root, output root, and CancellationToken; define these names once and reuse them.

- [ ] Write cooker tests for valid opaque base color and rejection of wrong platform/schema, texture binding, Lit=true, transparency, and unsupported vertex formats. Emit PlatformMaterialAsset with Lit=false, no texture, opaque alpha, and authored BaseColor channels. Keep vertex color disabled for the initial single-color authored material so the demo's rainbow colors cannot hide mistakes.
- [ ] Validate the resolved scene and dependency graph: exactly one supported camera and one mesh/material submission, supported transform, bounded geometry, no unsupported runtime components. Report scene/asset ID and unsupported capability. Reject missing references and invalid indices, NaNs/infinities, or unrepresentable coordinates before invoking the native compiler.
- [ ] Stage only manifest-declared cooked artifacts, preserving their declared relative paths and identity. Reject escaping paths, duplicates, and case-only collisions. Stage generated sources byte-for-byte; do not repair them after generation.
- [ ] Reuse shared serialized model/scene/material formats. Treat the graph's declared LittleEndian as the disk contract; verify generated deserializers decode it on the target. A big-endian CPU does not mean the payload should be raw memcpy'd into C++ structs or globally byte-swapped.
- [ ] Write a startup catalog from Manifest.StartupSceneId and actual scene entries. Prefer generated N64-owned header data or a small explicitly versioned catalog matching existing runtime catalog semantics. Builder-owned catalog generation is permitted; rewriting core-generated files is not. Escape IDs/paths correctly and reject missing startup ID.
- [ ] Package content into DragonFS with the selected SDK rules:

```makefile
# STAGED_ASSET_FILES is the actual declared list emitted by the builder.
N64_MKDFS_ROOT := $(HELENGINE_N64_FILESYSTEM_ROOT)
$(BUILD_DIR)/helengine_n64.dfs: $(STAGED_ASSET_FILES)
helengine_n64.z64: $(BUILD_DIR)/helengine_n64.dfs
```

Confirm when N64_MKDFS_ROOT must be defined relative to the SDK include. Track added, deleted, and renamed assets in dependency metadata; do not leave a stale DFS image after a rename.

- [ ] Replace provisional BuildAsync with real staged cooking/packaging execution. Propagate compiler and cancellation failures. Publish ROM/ELF/map/hash record only from successful output; failed builds cannot report an old ROM as current.
- [ ] Add tests with an injected native executor: complete expected staged asset set, explicit missing dependency failure, cancellation propagation, failed native build yields failure, rename removes old packaged path. Avoid requiring Docker for these unit tests.

**Gate B1:** a ROM contains exact staged fixture assets and its catalog. One renamed or missing required asset invalidates the build. No claim of scene loading yet.

**Commands:** `dotnet test builder.tests/helengine.n64.builder.tests.csproj`; one native package build through the installed image.

**Commit:** `feat(n64): cook unlit scene assets and package DragonFS`.

## Task 3 — Load the authored scene through Core

**Create:** `src/platform/n64/{Nintendo64RuntimeHost,Nintendo64ContentStreamSource,Nintendo64SceneBootstrap}.*` and only required generated-interface adapters identified in Task 1. Keep one class per file.

**Modify:** src/main.cpp and build mode selection; preserve Nintendo64BootHost diagnostic mode. Use shared SceneManager/content APIs with their verified generated signatures.

- [ ] Initialize video and diagnostics, call `dfs_init(DFS_DEFAULT_LOCATION)`, then bind the runtime content source to `rom:/`. Map cooked relative paths consistently without host path fallback. Close streams promptly; the documented DragonFS open-file limit is four.
- [ ] Install the builder-produced RuntimeSceneCatalog and use the selected startup scene ID. Initialize Core through normal options/backend contracts, then load/materialize the scene through SceneManager. Missing data stops with a specific error instead of falling back to the demo.
- [ ] Implement required adapters as narrow explicit capabilities. Disable the engine-owned performance/splash overlay through supported configuration when it would request unsupported 2D work. Do not add a silent no-op renderer that conceals actual UI submissions.
- [ ] Log `n64:core-ready`, startup scene ID, loaded model/material IDs, and camera/drawable counts once. Prove there is exactly one camera and one drawable before adding Tiny3D drawing.
- [ ] Add a target serialization probe using real cooked fixture bytes: known nontrivial position values, index values, and RGBA channels. Compare decoded values to expected values; verify both signed/float fields and lengths/indices, not just a file-open success.
- [ ] Verify missing catalog/file corruption paths with an isolated fixture build. Require clear diagnostics and no hardcoded demo fallback.

**Gate B2:** the generated runtime loads and extracts the authored objects, and target-side deserialization matches the fixture. If byte order is wrong, repair the shared serialization/codegen source with a focused test, not staged output.

**Commit:** `feat(n64): load authored scenes through the generated runtime`.

## Task 4 — Render Core's camera and mesh

**Create:** `src/platform/n64/{Nintendo64RenderManager3D,Nintendo64RuntimeModel,Nintendo64RuntimeMaterial,Nintendo64RenderQueueVisitor}.*`; add host-testable mesh conversion/math helpers only where needed.

**Modify:** Nintendo64Renderer.hpp/.cpp and Nintendo64RuntimeHost. Generalize drawing to persistent mesh resources and caller-provided model/camera data; retain DrawCube as the diagnostic wrapper.

- [ ] Derive the adapter from the actual generated RenderManager3D contract. Implement model/material build/release methods, capabilities, and Draw using the normal extracted camera/ordered drawable stream. Do not walk raw scene files directly to bypass Core.
- [ ] Convert supported cooked vertices to T3DVertPacked once at asset creation, enforcing the documented v0.2 limits. Define rounding, signed-16-bit range, color mapping, and winding conversion explicitly. Preserve authored units through an explicit model transform when quantization requires scaling; never clamp coordinates silently. Keep indices validated and resources resident.
- [ ] Test the real conversion helper with cube data, index bounds, limit overflow, negative coordinates, signed-range boundaries, and asymmetric transform data. Use host tests for pure conversion math and target tests for SDK/matrix packing.
- [ ] Read camera view/projection and drawable world transform from runtime submissions. Test matrix orientation with translation (3,1,0), nonuniform scale (1,2,0.5), and a rotation; a symmetric origin cube alone can hide transposition errors. Preserve near/far and clipping semantics.
- [ ] Apply authored base color, explicit NO_LIGHT, depth compare/write, backface culling, AA_NONE, no blend/fog after Tiny3D frame start. Face colors in the old demo must not leak into this material path.
- [ ] Assign one owner for BeginFrame/clear/presentation. The intended sequence is:

```text
Update Core with bounded elapsed seconds
Finish previous graphics work before modifying frame resources
Prepare authored camera and model data
Video.BeginFrame
Tiny3D frame start + authored viewport
Clear using authored camera clear color/depth
Core draw / N64 RenderManager3D submission
Tiny3D triangle sync
Video.EndFrame (detach_wait then display_show)
Retire resources only after completion
```

Map this sequence to actual Core.Update/Core.Draw behavior without double submission or double presentation. Keep resource release callbacks deferred until the known completion boundary.

- [ ] Add one startup log for first authored draw, including model/material IDs, and one completed-frame log. Disable hardcoded FrameClock rotation in authored mode; scene transforms should fully explain output.

**Gate C:** ROM shows the authored mesh and camera, using its material. Diagnostic cube still builds/runs. No unexpected allocation in the steady-state render loop.

**Commit:** `feat(n64): render generated runtime scene submissions with Tiny3D`.

## Task 5 — Editor-to-ROM proof and final evidence

**Create/modify:** sample generator, sample project assets, `scripts/verify-authored-scene.ps1`, `docs/AuthoredSceneBuild.md`, `docs/Validation.md`.

- [ ] Run the actual generic build route, using the chosen project path and installed development descriptor:

```powershell
& C:\dev\helworks\helengine\scripts\build-platform.ps1 `
  -Project "$n64Root\samples\AuthoredCube\project.heproj" `
  -Platform n64 -BuildProfile debug `
  -Output C:\dev\helworks\builds\helengine-n64\v02\authored-cube
if ($LASTEXITCODE -ne 0) { throw 'Authored N64 build failed.' }
```

The generated fixture tool must create a valid .heproj/scene/model/material using actual current serializers. The variable n64Root resolves to the implementation checkout; it is not an assumed main path.

- [ ] Baseline fixture: cube at (0,0,0), opaque red, fixed authored camera. Variant: cube at (3,1,0), opaque green. Produce both from authored data through the same pipeline, with no native source edits. Compare cooked content and ROM hashes, then inspect the actual rendered images. Hash changes alone do not prove rendering.
- [ ] Change the authored camera and verify the view changes. Repeat a clean build and an incremental color/transform change; changing SDK pins is unnecessary.
- [ ] Exercise unsupported material, missing mesh, and >64-vertex fixture failures. Confirm diagnostics identify the asset and capability; no partial successful ROM is published.
- [ ] Record native image identities, engine/N64/generator revisions, source dirty state, ROM hash, ELF footprint, runtime memory evidence, logs, emulator version, and exact tests. Require a supported 4 MiB execution configuration where available; mark untested hardware cases explicitly.
- [ ] Run only affected tests: N64 builder/host suites and focused shared-engine/generator regression tests for any changed common code. Build both authored and diagnostic modes. Inspect diffs across each repository before local commits; do not merge or push.

**Commit:** `test(n64): verify authored scene changes reach the ROM`.

## Implementation rules and cost budget

One Luna xhigh agent, sequential work. Report checkpoint outcomes; don't re-read both reference manuals on every step. Reuse generated output when inputs are unchanged; regenerate only when profile/source changes require it. Keep test output bounded and logs in the visible build root.

After two unsuccessful evidence-based fixes of one SDK/ABI, serialization, matrix, or runtime-memory failure: report the exact symptom, two tested hypotheses, relevant paths, and smallest next investigation. Request a revised plan or optional escalation; do not automatically use a larger model. Unavailable emulator/hardware is a separate verification limitation, not grounds for a model upgrade.

## Handoff prompt

> Implement this plan with gpt-5.6-luna at xhigh, sequentially and without other agents. Start from the completed N64 native host checkout and read the linked spec and applicable instructions. Reuse the existing build image. Prove minimal generated Core compatibility before implementing the whole scene path. Then package shared cooked assets into DragonFS, load through Core, and render its actual camera/mesh/material submissions. Demonstrate authored position/color/camera edits reaching the ROM. Preserve the diagnostic cube, 4 MiB target, and explicit graphics resource lifetime rules. Make local checkpoint commits without pushing. Report specific blockers rather than guessing or switching models.
