# N64 native host validation

## Authored build evidence

The corrected authored build was run from the N64 implementation checkout with the
pinned Docker image:

```powershell
pwsh -NoProfile -File scripts/build.ps1 -Clean -Mode authored `
  -GeneratedCoreRoot C:\dev\helworks\builds\helengine-n64\v02\generated\core-authored2 `
  -FilesystemRoot C:\dev\helworks\builds\helengine-n64\v02\filesystem-authored3
```

It linked the generated Core unity source, the N64 runtime adapters, the
builder-owned startup catalog, and a DragonFS image. The corrected checkpoint
produced:

- ROM SHA-256: `8e7969c28949606bddfbf809a8cd74bfb24953ffed680cc0ab95f17138903eb2`
- ROM bytes: `884736`
- ELF load image from the linker size report: text `816632`, data `204705`,
  bss `10224`, total `1031561`
- Toolchain image: `sha256:efceec9ed6d810b56388c508f4121597bfbba70d41a6dfeee4d8cad5f295fc32`
- Compiler: `mips64-elf-g++ (GCC) 16.2.0`
- Video: 320x240, 16-bit color, two buffers, 16-bit depth, `AA_NONE`

The staged fixture was generated through the editor `AssetSerializer` and
`EditorPlatformBuildScenePackager`, then placed under
`C:\dev\helworks\builds\helengine-n64\v02\filesystem-authored3`:

- `cooked/scenes/authored.hasset`: 754 bytes
- `cooked/engine/models/cube.hasset`: 913 bytes
- `cooked/engine/materials/standard.hasset`: 80 bytes

The package was reproduced with the checked-in
`tools/helengine.n64.samplegen` source through
`scripts/generate-authored-fixture.ps1`. Camera and mesh entities serialize
layer mask `1`; the generated runtime loader preserves that value so the mesh
enters the camera's 3D queue.

The generated Core was regenerated after cooking this scene. Its final
`helengine_core_unity.cpp` includes the generated camera and mesh component
deserializers, and the authored Makefile links that final unit instead of the
earlier `generated_unity.cpp`, which may have an empty component registry.
The startup catalog names `cooked/scenes/authored.hasset`, and the DFS image
contains that exact path plus both dependencies. The earlier
`filesystem-probe` tree is retained as evidence only; its
`scenes/authored.scene` file was five literal ASCII bytes (`probe`) and is not
used by the corrected ROM. The build script emitted the ROM and ELF
successfully; its post-build `Get-FileHash` bookkeeping step is unavailable in
the `rtk powershell` wrapper, so the SHA-256 above was computed independently.

## Regression checks

- `dotnet test builder.tests/helengine.n64.builder.tests.csproj --no-restore
  -p:BuildProjectReferences=false`: 8 passed.
- C++ generator writer/harness focused tests: 12 passed.
- Shared `AliasedContentStreamSourceTests`: 4 passed.
- `scripts/test-host.ps1`: frame-clock and N64 content-source tests passed;
  the latter verifies both low-level DFS calls receive paths without `rom:/`.
- `scripts/verify-authored-scene.ps1`: final generated Core registration,
  startup catalog, layer-mask source/loader checks, serialized scene header,
  and DFS path checks passed.
- Clean diagnostic mode rebuild: 342,045-byte ELF and a distinct ROM hash,
  preserving the standalone multicolored cube path.

## Runtime evidence

Gopher64 execution of the layer-corrected ROM produced the following target
trace, with no exception:

```text
scene-ready cameras=1 drawables3d=1
first-authored-draw model=engine:model:cube material=n64-unlit calls=1
first-frame-complete
```

HelenUI capture showed the authored white cube face against the configured dark
clear color. The final clean ROM uses the same serialized scene and runtime
path, with diagnostic queue tracing removed. Hardware execution remains
unverified.

## cube_test focused checks

- The regenerated N64 cooked scene contains two active roots and three
  components: the authored camera, mesh, and original
  `city.rendering.CubeTestSpinComponent`; unsupported UI, light, scaffold, and
  orbit content is excluded by explicit authored N64 overrides.
- `Float4CameraForwardTests` passes 2/2 in `helengine.core.tests`, covering
  identity `-Z` forward and a nonidentity yaw through the engine's own
  `float4.RotateVector` implementation.
- The focused preserving-override editor test passes 1/1.
- Standard package evidence is retained at
  `C:\dev\helworks\builds\helengine-n64\cube-test\probe-package-final`.

## cube_test ROM with generated Core (2026-09-15)

The existing demodisc `scenes/rendering/cube_test.helen` scene was built
through the normal editor pipeline with the regenerated shared codegen
executable (csharpcodegen commits `987b6b9`, `09b1f5b`, `6ea2819`: Math.Asin
and Math.Sign runtime surface, one- and two-argument character `Split`
overloads, and enum member names for omitted optional arguments). The final
build workspace is
`C:\dev\helworks\builds\helengine-n64\cube-test\work\n64\af84edd75b6b40dcbb726d5061a89c10`
(`generated-core` and `builder\n64-dragonfs`). The editor build log is
`normal-build-math-split-fixed.log` and the clean native build log is
`native-build-math-split-fixed.log`, both under
`C:\dev\helworks\builds\helengine-n64\cube-test`.

Native build command (run from this checkout; `pwsh` was not on the session
PATH, so the executor's `pwsh` launch failed and the native step was rerun with
the explicit PowerShell 7 path against the finalized workspace):

```powershell
pwsh -NoProfile -File scripts/build.ps1 -Clean -Mode authored `
  -GeneratedCoreRoot C:\dev\helworks\builds\helengine-n64\cube-test\work\n64\af84edd75b6b40dcbb726d5061a89c10\generated-core `
  -FilesystemRoot C:\dev\helworks\builds\helengine-n64\cube-test\work\n64\af84edd75b6b40dcbb726d5061a89c10\builder\n64-dragonfs
```

Final artifact, copied to
`C:\dev\helworks\builds\helengine-n64\cube-test\out\rom\helengine_n64_cube_test.z64`:

- ROM SHA-256: `332cfc04fc1a33696ae40a505e9f054ed8f50ba036f2dff6a1199b62e89e756d`
- ROM bytes: `9207808`
- ELF load image: text `856040`, data `210945`, bss `10272`, total `1077257`
- Toolchain image: `helengine-n64-toolchain:latest@sha256:efce4e8f134899c8f199c91c9eee2d26ae3bb562fc5d9702`

`build/helengine_n64.map` lists `CubeTestSpinComponent::Update()`,
`CubeTestSpinComponent::set_AngularSpeedRadians(float)`, and
`GeneratedRuntimeCubeTestSpinComponentDeserializer::Deserialize(...)` inside
`build/generated-core/helengine_core_unity.o`, so the original component runs
through generated Core rather than native animation.

Gopher64 stdout (`out\gopher-cube-stdout.log`) with no error lines:

```text
n64:core-ready
n64:startup-scene id=cube_test
n64:material-loaded id=Materials.rendering.cube_test.CubeTestSolid color=FFFFFFFF
n64:model-loaded id=engine:model:cube vertices=24 indices=36
n64:scene-ready cameras=1 drawables3d=1
n64:first-authored-draw model=engine:model:cube material=n64-unlit calls=1
n64:first-frame-complete
```

Rotation evidence (`out\frames`): five HelenUI captures 700 ms apart and eight
captures 250 ms apart. Every consecutive pair differs only inside the cube's
bounding box (roughly x 262-381, y 223-319 in the 640x480 window), between
0.3% and 0.8% of the window pixels per interval, and the silhouette changes
from a square face to a rotated outline with visible edges. The unlit white
material renders every face the same color, so only the silhouette moves.
Hardware execution remains unverified.

## cube_test lighting

Scene lighting was validated end to end on 2026-09-16 through the normal editor
build of `C:\dev\helprojs\demodisc\project.heproj` (`--build n64
--build-profile release`), logged to
`C:\dev\helworks\builds\helengine-n64\cube-test\normal-build-lighting.log`. The
editor completed generation, cooking, and DragonFS staging and then failed only
at the native step because `pwsh` is not on the editor process PATH. The native
build was rerun directly against the finalized workspace
`5e4a87850e7c4a52aa6325ca4edd70f5`:

```powershell
pwsh -NoProfile -File scripts/build.ps1 -Clean -Mode authored `
  -GeneratedCoreRoot C:\dev\helworks\builds\helengine-n64\cube-test\work\n64\5e4a87850e7c4a52aa6325ca4edd70f5\generated-core `
  -FilesystemRoot C:\dev\helworks\builds\helengine-n64\cube-test\work\n64\5e4a87850e7c4a52aa6325ca4edd70f5\builder\n64-dragonfs
```

The cooked scene staged to
`cooked/scenes/rendering/cube_test.hasset` and its material to
`cooked/materials/rendering/cube_test/cubetestsolid.hasset`.

Final artifact, copied to
`C:\dev\helworks\builds\helengine-n64\cube-test\out\rom\helengine_n64_cube_test_lit.z64`:

- ROM SHA-256: `b5875d3990666abd48cf59d12a827401e96279bb0ca867245e39aee06de869e6`
- ROM bytes: `9224192`
- ELF load image: text `873128`, data `214113`, bss `10304`, total `1097545`

Gopher64 stdout (`out\gopher-lit-stdout.log`) contained no `n64:error` line:

```text
n64:authored-runtime-start
n64:video-ready
n64:renderer-ready
n64:dragonfs-ready
n64:core-ready
n64:startup-scene id=cube_test
n64:material-loaded id=Materials.rendering.cube_test.CubeTestSolid color=FFFFFFFF lit=1
n64:model-loaded id=engine:model:cube vertices=24 indices=36
n64:scene-ready cameras=1 drawables3d=1 directional=1 ambient=0
n64:first-authored-draw model=engine:model:cube material=n64-standard lit=1 lights=1 calls=1
n64:first-frame-complete
```

`lit=1` on both the cooked material and the draw, and `directional=1 ambient=0`,
confirm the cook kept `CubeTestSun` and that the material is lit by default.

### Shading evidence

Eight captures 250 ms apart in `out\frames-lit`. Distinct non-background
luminance levels inside the cube crop `(240, 200, 400, 330)`:

```text
frame01 levels=10 min/max=(44, 144)
frame02 levels= 7 min/max=(84, 176)
frame03 levels= 5 min/max=(72, 136)
frame04 levels= 9 min/max=(56, 152)
frame05 levels= 3 min/max=(84, 168)
frame06 levels= 2 min/max=(76, 136)
frame07 levels= 7 min/max=(48, 144)
frame08 levels= 6 min/max=(84, 176)
```

Every frame has at least two non-background levels separated by more than 40,
and no level saturates at 255. The previous unlit ROM rendered the whole cube at
a single flat white level, so this is the shading change. Per-face bands (the
cube's visible faces are flat, and the spread within a band is the 5-bit
framebuffer dither) show adjacent faces at clearly different levels, for example
frame01 at ~70 and ~143 and frame05 at 0 and 168. A face reaching exactly 0
confirms there is no built-in ambient term.

Motion (`frame_diff.py`) still passes: each consecutive pair differs by 2.5% to
3.4% of window pixels, and every difference bounding box is inside the cube
region x 262-380, y 223-319.

### Light direction sign

The authority for the sign is Tiny3D's own convention, not the captured frames.
In `.deps/tiny3d/examples/02_lighting/main.c` lines 103-109 the example places
the visible light marker at `dir * 20`, so the vector handed to
`t3d_light_set_directional` points from the surface toward the light, not from
the light toward the surface. `.deps/tiny3d/src/t3d/rsp/rsp_tiny3d.rspl` lines
409-424 confirms it in the ucode: the vertex normal is multiplied with the
stored light vector (`lightDirScale = norm:sfract * lightDirVec:sfract`) and the
products are accumulated into the light color with no negation anywhere in the
loop, so a surface facing the light must produce a positive dot product. That is
why `Nintendo64LightMath::ToLightVector` negates the entity forward axis:
`CubeTestSun` is authored with yaw -0.65 and pitch -0.85 radians, its world
forward is `(0.399, -0.751, -0.525)` - mostly downward - and the negated
surface-to-light vector `(-0.399, 0.751, 0.525)` points up toward the sun. The
host test `TestLightVectorIsNegatedAndNormalized` pins that negation, and
`scripts/test-host.ps1` reports `n64-light-math-tests: PASS`.

The brief's by-eye "top face brighter than the sides" check is not observable on
this scene: the authored camera sits level with the cube centre and
`CubeTestSpinComponent` rotates yaw only, so only the four side faces are ever
rasterised - frame05 shows exactly two bands, one face and one background, with
no third band for a top face. The substitute measurement is the peak side face
luminance. Side-face normals are horizontal, so they can only receive the
horizontal part of the light, `cos(0.85) = 0.660`, predicting a peak of
`0.660 * 255 = 168`; the measured peak across the sweep is 168 to 176, within
one 5-bit framebuffer step (8 levels) of the prediction. This is a magnitude
check only. Because the cube spins through a full turn, every side-face normal
sweeps all horizontal headings, so the set of peak luminances is invariant under
a sign flip of the light vector - flipping the sign only decides which heading
peaks, not how bright the peak is. The measurement therefore confirms that the
remaining `sin(0.85) = 0.751` of the light sits on the vertical axis the side
faces cannot see, but it cannot by itself distinguish the sign. The sign rests
on the Tiny3D sources cited above.

Hardware execution remains unverified.

### Final branch ROM

After the final review fixes (`324dae4`, `fb57b50`: vertex-slot packing moved
into the host-tested `Nintendo64VertexPacking` seam, zero-length normals
rejected at load, host guard uses `kMaxDirectionalLights`), the ROM was rebuilt
from the same generated workspace and booted again in Gopher64:

- ROM: `C:\dev\helworks\builds\helengine-n64\cube-test\out\rom\helengine_n64_cube_test_lit_final.z64`
- SHA-256: `4814b7a34aa1a7cc7ffc9c953dfd1f95c5a1266f96f8a37a4d5eaee6d289a142`
- Bytes: `9224192`
- Stdout (`out\gopher-lit-final-stdout.log`) is line-for-line identical to the
  earlier lit run: `lit=1`, `directional=1 ambient=0`, `lights=1`,
  `first-frame-complete`, no `n64:error`.
- Four captures 300 ms apart (`out\frames-lit-final`) each differ inside the
  cube bounding box only (2.2% to 3.1% of window pixels per interval), so the
  packing refactor left the rendered output and rotation unchanged.

## cube_test sprites

The demodisc `cube_test` scene no longer excludes the shared console
camera/light instruction Blueprint on N64 (demodisc `88f8326`). The regenerated
`assets/scenes/rendering/cube_test.helen` keeps only the `dc` existence override
on that root, and `CubeTestSun` carries no override at all.

- ROM: `C:\dev\helworks\builds\helengine-n64\cube-test\out\rom\helengine_n64_cube_test_sprites.z64`
- SHA-256: `c76898beca48b7186187c79251c70781fbf4b583e472542f9727ecf2cf4c65d7`
- Bytes: `1589248`
- Build workspace: `22ca7b9ba847446c9270e583a1079321`

The first ROM to render sprites was
`ed49ccd8b655dafe7f4e455d91057560f0a48bb4f05036bd7c7aceed644d378c`. Extracting
the sprite geometry into the host-tested `Nintendo64SpriteGeometry` seam relinked
it to the hash above with no behaviour change: the boot trace is line for line
identical, and the overlay-pixel count below reproduces at exactly 932 in the
same region. The evidence in this section is quoted from the relinked ROM.

### Build

Runtime feature manifest:

```
Enabled runtime features: render2d (AutoDetected), sprites (AutoDetected). Disabled runtime features: debug_overlay (ForcedDisabled), host_file_system (NotIncluded), managed_metadata_only (ForcedDisabled), physics3d.diagnostics (NotIncluded), reflection_like_runtime (NotIncluded), shaders (ForcedDisabled), text2d (ForcedDisabled), text_processing (NotIncluded).
```

All 32 builder-owned cook work items completed, ending with
`runtime-texture:cooked/fonts/default.hetex`. DragonFS staging wrote 38 files
and 536741 bytes, including 30 `cooked/imported/<hash>` textures and both cooked
font atlases. Every `cooked/imported/*` payload is 16526 bytes: a 142-byte asset
header plus 64*64*4 straight RGBA32, confirming the max-64-pixel downscale cook.

The cooked scene carries exactly three `helengine.SpriteComponent` records, all
with two positive `Size` axes, so no sprite falls back to its source rect on a
single axis:

| Entity | Cooked texture | Size | SourceRect |
| --- | --- | --- | --- |
| `CameraIconPrimary` | `cooked/imported/568d5f9c...` | 48x48 | 0.1406, 0.1406, 0.7227, 0.7227 |
| `CameraIconSecondary` | `cooked/imported/34795cd0...` | 38x48 | 0.2109, 0.0781, 0.5820, 0.7275 |
| `LightIcon` | `cooked/imported/cc1e9bd8...` | 46x46 | 0.2109, 0.2109, 0.5820, 0.5820 |

### Boot trace

`out\gopher-sprites-stdout.log`, verbatim:

```
n64:authored-runtime-start
n64:video-ready
n64:renderer-ready
n64:dragonfs-ready
n64:core-ready
n64:startup-scene id=cube_test
n64:texture-loaded id=runtime-texture:cooked/imported/568d5f9ca3b6b3b30b85dfef3ca6746324806b161c8e79298069b4a50878468c size=64x64
n64:texture-loaded id=runtime-texture:cooked/imported/34795cd0703804daa63e3f9eb76b7a644b1baaab10bcaadc6889f6791f257758 size=64x64
n64:texture-loaded id=runtime-texture:cooked/fonts/demodiscbody.hetex size=64x64
n64:texture-loaded id=runtime-texture:cooked/imported/cc1e9bd820dda8c73ad90fde92ee40595d67460755740c344e91130a22940f30 size=64x64
n64:material-loaded id=Materials.rendering.cube_test.CubeTestSolid color=FFFFFFFF lit=1
n64:model-loaded id=engine:model:cube vertices=24 indices=36
n64:scene-ready cameras=1 drawables3d=1 drawables2d=6 directional=1 ambient=0
n64:first-authored-draw model=engine:model:cube material=n64-standard lit=1 lights=1 calls=1
n64:first-sprite-draw size=12x12 calls=1
n64:first-frame-complete
```

`n64:first-sprite-skipped` does not appear, so no sprite resolved to an empty
destination, and there is no `n64:error` line. `drawables2d=6` is the three
sprites plus the two text labels and the panel rounded rect, which the runtime
accepts into the queue and draws as no-ops.

`size=12x12` is the destination extent, not a defect: the overlay is authored
against a reference canvas four times wider than the 320x240 frame buffer, so
the 48x48 authored icon scales to 12x12 frame-buffer pixels, shown at 24x24 in
the 2x emulator window.

### Frame evidence

Eight captures 250 ms apart in `out\frames-sprites`. Every consecutive pair
differs, and every difference bounding box lies inside the cube's bounding box
(x 262-381, y 223-319), so the cube still rotates and the overlay is static:

| Pair | Changed pixels | Share | Difference bbox |
| --- | --- | --- | --- |
| 1 -> 2 | 7076 | 2.3034% | 262, 223, 380, 319 |
| 2 -> 3 | 1296 | 0.4219% | 262, 223, 333, 319 |
| 3 -> 4 | 1304 | 0.4245% | 262, 223, 345, 319 |
| 4 -> 5 | 1336 | 0.4349% | 262, 225, 353, 319 |
| 5 -> 6 | 8784 | 2.8594% | 266, 225, 365, 319 |
| 6 -> 7 | 8112 | 2.6406% | 270, 225, 377, 319 |
| 7 -> 8 | 8760 | 2.8516% | 262, 223, 377, 319 |

Overlay-pixel check, counting pixels more than 12 in summed channel distance
from the captured clear colour `(16, 16, 24)`, inside the emulator client area
(x 9-633, y 31-479, which excludes the window chrome) and outside the cube
bounding box:

- frames 1, 2, 5 and 8 each report **932** pixels, in the region
  x 23-68, y 365-424.
- The count is identical in every frame, as a static overlay should be, and far
  above the 200 the lit-only ROM would have to beat.

The host clears to `RGBA32(16, 18, 24, 255)`
(`Nintendo64Video::Clear`). Captures read it back as `(16, 16, 24)` because the
RGBA5551 frame buffer keeps only the top five bits of each channel, so green 18
quantizes to 16. The count is taken against the captured value, which is what a
screenshot can actually contain.

Read directly, frame 1 shows the cube at roughly 45 degrees with a bright left
face and a dark right face, and three icons in the lower left: a D-pad cross, a
round analog stick in a circular housing beside it, and below them a dark round
button carrying a red circle glyph. Frame 5 shows the cube rotated nearly
face-on, a bright front face with a thin dark sliver of side wall, while the
three icons are pixel-identical and in the same place.

**The button art is PlayStation, not N64.** The red circle is the PS2 Circle
button and the D-pad plus single analog stick is the PS2 layout. The Blueprint's
icon entities carry a `ps2` baseline sprite with per-platform overrides for
`gamecube`, `wii`, `switch` and `wiiu` only; there is an `n64` family in
`assets/textures/instructions/controls/generated/manifest.json`, but no
`DesktopInstructionPlatformIconSpec` rows reference it, so N64 inherits the ps2
baseline. This proves the sprite path end to end; the art still needs N64 rows.

Neither instruction label nor the panel rounded rect is drawn, as expected:
`text2d` is forced off and `DrawRoundedRect` is a deliberate no-op.

### Regenerated Core differences

Against the last known-good workspace (`5e4a87850e7c4a52aa6325ca4edd70f5`), the
regenerated Core has 800 files instead of 807:

- **Added (11):** `GeneratedRuntimeSpriteComponentDeserializer`,
  `GeneratedRuntimeTextComponentDeserializer`,
  `GeneratedRuntimeRoundedRectComponentDeserializer` and
  `GeneratedRuntimeViewportComponentDeserializer` (header and source each),
  which follow directly from enabling `render2d` and `sprites`, plus the three
  new runtime support headers `runtime/native_runtime.hpp`,
  `runtime/native_allocation.hpp` and `runtime/native_memory_ops.hpp`.
- **Removed (18):** shader and text content-processing types that the newer
  capability-aware codegen prunes - `ShaderCompileTarget`,
  `ShaderContentProcessorBase_1`, `ShaderTargetDescriptor(s)`,
  `StandardShaderVariant(s)`, `StandardShaderVariantMakefileManifestWriter`,
  `TextContentProcessor` and `TextContentManagerConfiguration`. `text_processing`
  correspondingly moved from `AutoDetected` to `NotIncluded`; the font atlases
  are still cooked and staged, and the runtime still loads
  `cooked/fonts/demodiscbody.hetex` as a texture.
- **Size-changed (164):** almost all of these are a single added
  `#include "runtime/native_string.hpp"` line, plus renumbered
  `__nativeOwnershipTarget_*` and `__ctor_arg_*` temporaries. The one
  behavioural change spotted is in `Core.cpp`, which gained an early
  `if (this->RenderManager3D == nullptr) { return 0.0; }` guard - that is
  helengine `c0157e99`, not a codegen difference.

### Toolchain note

This ROM was produced with an isolated csharpcodegen master worktree at
`C:\dev\helworks\builds\helengine-n64\codegen-master` (`b477b70`), registered
for the `n64` platform row only. The shared
`csharpcodegen\codegen\bin\Release` output was a day stale at the time and
lacked `runtime/native_runtime.hpp`; building N64 against it failed with
`'HeCppString' does not name a type`. That gap is closed - the shared
executable has since been rebuilt and the ROM reproduces against it, as
recorded in the next section.

## cube_test on the shared code generator

The reproducibility gap above is closed. The shared
`C:\dev\helworks\csharpcodegen\codegen\bin\Release\net9.0\codegen.exe` was
rebuilt from csharpcodegen `master` at `c3c0521` and now carries both
`runtime/native_runtime.hpp` and `runtime/native_endian.hpp`. The `n64` platform
row in `C:\dev\helworks\helengine\user_settings\platforms.json` points at that
executable and at the merged `C:\dev\helworks\helengine-n64` checkout, so N64 no
longer needs a privately built generator. Regenerated and rebuilt from
`helengine-n64` `main` at `09f8a4c`.

- Shared generator: `C:\dev\helworks\csharpcodegen\codegen\bin\Release\net9.0\codegen.exe`,
  csharpcodegen `master` `c3c0521`
- ROM: `C:\dev\helworks\builds\helengine-n64\cube-test\out\rom\helengine_n64_cube_test_shared_codegen.z64`
- SHA-256: `7e8fa1e44fd9b106f91b7ba85f52a41e7e15fe5bb443bec8bb3bfce6fec00fad`
- Bytes: `1589248` - the same size as the previous ROM, a different hash
- Build workspace: `110a2a3cfd824828bc3040be7e04a32b`
- ELF load image: text `979352`, data `234177`, bss `10328`, total `1223857`

The build ran as the normal editor project build; the native step was run
separately because the editor's build executor launches `pwsh`, which is not on
this machine's PATH:

```powershell
$env:HELENGINE_N64_REPOSITORY_ROOT = 'C:\dev\helworks\helengine-n64'
$env:HELENGINE_BUILD_WORKSPACE_ROOT = 'C:\dev\helworks\builds\helengine-n64\cube-test\work'
dotnet C:\dev\helworks\helengine\helengine.ui\helengine.editor.app\bin\Debug\net9.0-windows\helengine.editor.app.dll `
  --project C:\dev\helprojs\demodisc\project.heproj `
  --build n64 --build-profile release `
  --output C:\dev\helworks\builds\helengine-n64\cube-test\editor-output

pwsh -NoProfile -File scripts/build.ps1 -Clean -Mode authored `
  -GeneratedCoreRoot ...\work\n64\110a2a3cfd824828bc3040be7e04a32b\generated-core `
  -FilesystemRoot ...\work\n64\110a2a3cfd824828bc3040be7e04a32b\builder\n64-dragonfs
```

The merged checkout had no `.deps`, which is gitignored, so the first native run
stopped on `Pinned dependencies are missing`. The libdragon and tiny3d checkouts
were copied in from `helengine-n64-impl` at exactly the revisions
`dependencies.json` pins (`39d0d609`, `73d822f3`).

The runtime feature manifest is unchanged from the previous ROM:

```
Enabled runtime features: render2d (AutoDetected), sprites (AutoDetected). Disabled runtime features: debug_overlay (ForcedDisabled), host_file_system (NotIncluded), managed_metadata_only (ForcedDisabled), physics3d.diagnostics (NotIncluded), reflection_like_runtime (NotIncluded), shaders (ForcedDisabled), text2d (ForcedDisabled), text_processing (NotIncluded).
```

All 32 cook work items completed and DragonFS staging wrote 38 files and 536741
bytes, both identical to the previous build.

### Boot trace

`out\gopher-shared-codegen-stdout.log`, verbatim:

```
n64:authored-runtime-start
n64:video-ready
n64:renderer-ready
n64:dragonfs-ready
n64:core-ready
n64:startup-scene id=cube_test
n64:texture-loaded id=runtime-texture:cooked/imported/568d5f9ca3b6b3b30b85dfef3ca6746324806b161c8e79298069b4a50878468c size=64x64
n64:texture-loaded id=runtime-texture:cooked/imported/34795cd0703804daa63e3f9eb76b7a644b1baaab10bcaadc6889f6791f257758 size=64x64
n64:texture-loaded id=runtime-texture:cooked/fonts/demodiscbody.hetex size=64x64
n64:texture-loaded id=runtime-texture:cooked/imported/cc1e9bd820dda8c73ad90fde92ee40595d67460755740c344e91130a22940f30 size=64x64
n64:material-loaded id=Materials.rendering.cube_test.CubeTestSolid color=FFFFFFFF lit=1
n64:model-loaded id=engine:model:cube vertices=24 indices=36
n64:scene-ready cameras=1 drawables3d=1 drawables2d=6 directional=1 ambient=0
n64:first-authored-draw model=engine:model:cube material=n64-standard lit=1 lights=1 calls=1
n64:first-sprite-draw size=12x12 calls=1
n64:first-frame-complete
```

This is byte-for-byte the trace the previous ROM produced: same four
`n64:texture-loaded` lines with the same ids, the same `n64:scene-ready` counts,
the same `n64:first-sprite-draw size=12x12 calls=1`, `n64:first-frame-complete`,
and neither `n64:error` nor `n64:first-sprite-skipped`. Asset loading is
therefore unaffected by the byte-order change, which is what the
`EngineBinaryReader` design predicts.

### Frame evidence

Eight captures 250 ms apart in `out\frames-shared-codegen`. Every consecutive
pair differs and every difference bounding box lies inside the cube's bounding
box (x 262-381, y 223-319), so the cube still rotates and the overlay is static:

| Pair | Changed pixels | Share | Difference bbox |
| --- | --- | --- | --- |
| 1 -> 2 | 580 | 0.1888% | 264, 225, 353, 319 |
| 2 -> 3 | 2490 | 0.8105% | 262, 223, 353, 319 |
| 3 -> 4 | 12 | 0.0039% | 280, 315, 309, 319 |
| 4 -> 5 | 1940 | 0.6315% | 262, 223, 349, 319 |
| 5 -> 6 | 2140 | 0.6966% | 264, 225, 363, 319 |
| 6 -> 7 | 1360 | 0.4427% | 270, 225, 369, 319 |
| 7 -> 8 | 9934 | 3.2337% | 262, 223, 369, 319 |

The per-pair counts are lower than the previous run's and pair 3 -> 4 changed
only 12 pixels. Neither is a regression: the capture cadence is wall-clock and
is not phase-locked to the spin, so consecutive captures can land closer
together in the rotation than they did before. What matters is that no pair is
identical, so no frame is frozen, and that every bbox stays inside the cube.

Overlay-pixel check, counting pixels more than 12 in summed channel distance
from the captured clear colour `(16, 16, 24)`, inside the emulator client area
(x 9-633, y 31-479) and outside the cube bounding box:

- frames 1, 2, 5 and 8 each report **932** pixels, in the region
  x 23-68, y 365-424.

That is the same count in the same region as the previous ROM, to the pixel.

Read directly, frame 1 shows the cube at roughly 45 degrees with a bright left
face and a dark right face, and the three icons in the lower left: a D-pad
cross, a round analog stick beside it, and below them a dark round button
carrying a red circle glyph. Frame 8 shows the cube rotated further, with a
wider dark side wall, while the three icons are pixel-identical and in the same
place. The button art is still the PS2 baseline, unchanged and expected.

### Regenerated Core differences

Against the previous workspace (`22ca7b9ba847446c9270e583a1079321`), the
regenerated Core has one more file, 801 versus 800 counting sources and headers:

- **Added (1):** `runtime/native_endian.hpp`, from csharpcodegen `8ae4e55`.
- **Removed:** none.
- **Size-changed (56):** four carry the real changes, the rest are noise.
  - `system/io/binary-reader.cpp` and `system/io/binary-writer.cpp` replaced the
    `std::reverse`-when-big-endian branch with
    `he_cpp_endian::NeedsSwap(littleEndian)`, which swaps only when the wire and
    host orders disagree. On this big-endian target reading little-endian cooked
    data the two disagree, so the generated reader now swaps where it previously
    copied straight through. Nothing in the boot path notices, because cooked
    assets go through the engine's `EngineBinaryReader` and its
    `BinaryReaderBE`/`BinaryReaderLE` subclasses, which assemble values byte by
    byte and never call the generated `BinaryReader::Read<T>`.
  - `system/bit_converter.hpp` gained `BitConverter::IsLittleEndian`, a
    `constexpr` constant with no call sites here.
  - `CameraComponent`, `CameraProjectionUtils` and `ICamera` gained a field of
    view clamp and a `CreatePerspectiveProjection(camera, aspectRatio)`
    overload. That is a helengine source change, not a codegen difference, and
    it is behaviour-neutral for this scene because the authored field of view is
    inside the clamp range.
  - The remaining ~49 files shrank by a handful of bytes each. These are the
    static runtime support headers the generator copies verbatim; the shared
    Release build emits them with LF line endings where the worktree build
    emitted CRLF. No content change.
- **The hash-lowering change is a no-op for this Core.** csharpcodegen `7d6a245`
  routes an inherited `object.GetHashCode()` through `he_cpp_get_hash_code`, but
  the helper appears in exactly the same three files with the same counts before
  and after (`FiniteStateTransitionKey_1.cpp` twice, `runtime/native_hash.hpp`
  and `runtime/native_hash_set.hpp` once each), so no call site in the N64 Core
  was affected.

`runtime/native_endian.hpp` as emitted derives the host order from
`__BYTE_ORDER__` alone and carries no `static_assert` against
`HE_CPP_PLATFORM_IS_LITTLE_ENDIAN`, confirming csharpcodegen `d8ad75d` is in the
shared build. Without it this configuration - a big-endian CPU declaring
little-endian serialization - would have failed the build on that assert.

The ROM hash differs from the previous one, which is expected: the generated
Core is not byte-identical, so the compiled image cannot be. The ROM size is
unchanged at 1589248 bytes, and every observable - trace, overlay pixels,
rotation - reproduces.

## cube_test on the newest shared code generator (2026-09-17)

Re-verification only: no N64 source or runtime change. Since the ROM above was
generated, csharpcodegen `master` advanced and the shared Release generator was
rebuilt from it. Two generic changes now apply that the recorded Core predates:

- `4f11363` "fix(cpp): bind delegates through the runtime instead of
  `std::bind_front`"
- `e47a165` "fix: keep a property's virtual modifier through the
  expression-bodied path"

Regenerated and rebuilt from `helengine-n64` `main` at `0123aa7`.

- Shared generator: `C:\dev\helworks\csharpcodegen\codegen\bin\Release\net9.0\codegen.exe`,
  csharpcodegen `master` `e47a165`
- ROM: `C:\dev\helworks\builds\helengine-n64\cube-test\out\rom\helengine_n64_cube_test_newest_codegen.z64`
- SHA-256: `58246706fb3b07ae62a3fcd1de013646f954a409bd4ca99b589fa78d97a7985e`
- Bytes: `1589248` - the same size as the previous ROM, a different hash
- Build workspace: `d0a8114bd20f4f66b07843b6ad35554e`
- ELF load image: text `979320`, data `234237`, bss `10328`, total `1223885`
  (previous: text `979352`, data `234177`, total `1223857` - `.text` shrank by
  32 bytes and `.data` grew by 60, consistent with three getters gaining vtable
  slots and the bind helper changing shape)

The build ran the same way as the previous entry: the editor project build, then
the native step separately because the editor's build executor launches `pwsh`,
which is not on this machine's PATH. `libdragon` and `Tiny3D` are now git
submodules under `.deps` at the pinned revisions `39d0d609` and `73d822f3`, so
no manual dependency copy was needed. The native build ran with `-Clean` and
completed with no errors; the only compiler diagnostics are the pre-existing
`system/io/file-stream.cpp` maybe-uninitialized notes.

The runtime feature manifest, the 32 cook work items and the DragonFS staging
(38 files, 536741 bytes) are all identical to the previous build.

### Boot trace

`out\gopher-newest-codegen-stdout.log`, verbatim:

```
n64:authored-runtime-start
n64:video-ready
n64:renderer-ready
n64:dragonfs-ready
n64:core-ready
n64:startup-scene id=cube_test
n64:texture-loaded id=runtime-texture:cooked/imported/568d5f9ca3b6b3b30b85dfef3ca6746324806b161c8e79298069b4a50878468c size=64x64
n64:texture-loaded id=runtime-texture:cooked/imported/34795cd0703804daa63e3f9eb76b7a644b1baaab10bcaadc6889f6791f257758 size=64x64
n64:texture-loaded id=runtime-texture:cooked/fonts/demodiscbody.hetex size=64x64
n64:texture-loaded id=runtime-texture:cooked/imported/cc1e9bd820dda8c73ad90fde92ee40595d67460755740c344e91130a22940f30 size=64x64
n64:material-loaded id=Materials.rendering.cube_test.CubeTestSolid color=FFFFFFFF lit=1
n64:model-loaded id=engine:model:cube vertices=24 indices=36
n64:scene-ready cameras=1 drawables3d=1 drawables2d=6 directional=1 ambient=0
n64:first-authored-draw model=engine:model:cube material=n64-standard lit=1 lights=1 calls=1
n64:first-sprite-draw size=12x12 calls=1
n64:first-frame-complete
```

`diff` against `out\gopher-shared-codegen-stdout.log` reports no differences: it
is byte-for-byte the previous ROM's trace. Four `n64:texture-loaded` lines, the
same `n64:scene-ready` counts, `n64:first-sprite-draw size=12x12 calls=1`,
`n64:first-frame-complete`, and neither `n64:error` nor
`n64:first-sprite-skipped` anywhere in stdout or stderr.

### Frame evidence

Eight captures 250 ms apart in `out\frames-newest-codegen`. Every consecutive
pair differs and every difference bounding box lies inside the cube's bounding
box (x 262-381, y 223-319):

| Pair | Changed pixels | Share | Difference bbox |
| --- | --- | --- | --- |
| 1 -> 2 | 9568 | 3.1146% | 262, 223, 365, 319 |
| 2 -> 3 | 872 | 0.2839% | 270, 225, 365, 319 |
| 3 -> 4 | 2640 | 0.8594% | 262, 223, 361, 319 |
| 4 -> 5 | 1098 | 0.3574% | 270, 223, 343, 319 |
| 5 -> 6 | 512 | 0.1667% | 274, 223, 333, 319 |
| 6 -> 7 | 7570 | 2.4642% | 262, 223, 380, 319 |
| 7 -> 8 | 10304 | 3.3542% | 266, 223, 380, 319 |

The per-pair counts differ from the previous run's, as they always do: the
capture cadence is wall-clock and is not phase-locked to the spin. What matters
is that no pair is identical, so no frame is frozen, and that every bbox stays
inside the cube.

Overlay-pixel check, counting pixels more than 12 in summed channel distance
from the captured clear colour `(16, 16, 24)`, inside the emulator client area
(x 9-633, y 31-479) and outside the cube bounding box:

- all eight frames report **932** pixels, in the region x 23-68, y 365-424.

That is the same count in the same region as the previous two ROMs, to the
pixel, and it is confirmed here on every frame rather than on a sample of four.

Read directly, frame 1 shows the cube at roughly 45 degrees with a bright left
face and a dark right side wall, and the three icons in the lower left: a D-pad
cross, a round analog stick beside it, and below them a dark round button
carrying a red circle glyph. Frame 8 shows the cube rotated to nearly face-on, a
flat mid-grey square with no visible side wall, while the three icons are
pixel-identical and in the same place.

### Regenerated Core differences

Against the recorded workspace (`110a2a3cfd824828bc3040be7e04a32b`), the
regenerated Core has the same 802 files, none added and none removed, and only
six differ in content.

**The delegate-binding change.** `runtime/native_runtime.hpp` gained
`he_cpp_bind_front`, a two-argument bind helper. When the standard library
advertises `__cpp_lib_bind_front >= 201907L` it forwards to `std::bind_front`;
otherwise it returns a lambda capturing the callable and the receiver by value
and calling `std::invoke(callable, bound, arguments...)`.

`SceneManager.cpp` (5 sites) and `RuntimeSceneLoadOperation.cpp` (1 site) are
the only call sites it touched. Each is a `new Action<T>(...)` construction over
a pointer-to-member plus a receiver, and each swapped `std::bind_front(...)` for
`he_cpp_bind_front(...)` with identical arguments. For example, in
`SceneManager::SceneManager`:

```cpp
// before
this->OwnedTextureTable = new ::SceneOwnedAssetReferenceTable_1<::RuntimeTexture*>(
    "runtime texture",
    new Action<RuntimeTexture*>(std::bind_front(
        static_cast<void (SceneManager::*)(RuntimeTexture*)>(&SceneManager::ReleaseOwnedAsset), this)));

// after
this->OwnedTextureTable = new ::SceneOwnedAssetReferenceTable_1<::RuntimeTexture*>(
    "runtime texture",
    new Action<RuntimeTexture*>(he_cpp_bind_front(
        static_cast<void (SceneManager::*)(RuntimeTexture*)>(&SceneManager::ReleaseOwnedAsset), this)));
```

No `std::bind_front` remains outside the helper. The N64 toolchain does
advertise `__cpp_lib_bind_front`, so the helper takes the forwarding branch and
the emitted behaviour is unchanged; the fallback exists for targets whose C++20
library is incomplete.

**Event subscription was not affected.** The obvious worry was
`ViewportComponent`, which subscribes to `RenderManager3D::WindowResized` and is
the component the instruction overlay in this ROM renders through. That path
goes through `Event::Bind`, not `std::bind_front`, so it is untouched:
`ViewportComponent.cpp` is byte-identical before and after, and line 398 reads
the same in both Cores.

```cpp
this->OwnerCore->RenderManager3D->WindowResized += Event::Bind(this,
    static_cast<void (ViewportComponent::*)(intptr_t, int32_t, int32_t)>(&ViewportComponent::HandleWindowResized));
```

The same holds for the other two `WindowResized` subscribers in this Core,
`LayoutComponent` and `ReferenceCanvasFitComponent`. The delegate change reaches
`Action<T>` construction from a receiver, not `+=`/`-=` on an `Event`.

**The virtual-property change.** `Component.hpp` and `RenderManager3D.hpp` carry
it: `Component::get_IsEditorUpdateExecutionSuppressionMarker`,
`Component::get_IsEditorSceneCameraSuppressionMarker` and
`RenderManager3D::get_LastDrawCallCount` gained `virtual`, restoring the
modifier their expression-bodied C# declarations carry. No `.cpp` changed with
them. This is behaviour-neutral here: the two marker getters have no call sites
at all in the N64 Core, and every derived component that redeclares them does
nothing but `return Component::get_...();` over a base that returns `false`, so
making the dispatch virtual cannot change a result. `get_LastDrawCallCount` has
no override in the Core, and its one caller, `Core.cpp:794`, reaches it through
a concrete `RenderManager3D*`.

`cpp-conversion-report.json` is the sixth differing file and differs only in the
absolute workspace paths it records.

The ROM hash differs from the previous one, which is expected: the generated
Core is not byte-identical, so the compiled image cannot be. The ROM size is
unchanged at 1589248 bytes, and every observable - trace, overlay pixels,
rotation, icon art - reproduces exactly.

## Rounded-rectangle fill in the 2D pass (2026-09-17)

The second 2D step: `RenderManager2D::DrawRoundedRect` no longer returns without
drawing. Sprites shipped two entries above; text is still a stub.

### Build

No code generation was re-run. Nothing that feeds codegen changed - only
`src/platform/n64` and the host test suite - so the ROM was built against the
same workspace the previous entry recorded, `d0a8114bd20f4f66b07843b6ad35554e`,
which isolates the render change from any generator drift.

- Host tests: all seven suites pass, the seventh being the new
  `n64-rounded-rect-geometry-tests`
- Native build: `scripts/build.ps1 -Clean -Mode authored` against that
  workspace's `generated-core` and `builder\n64-dragonfs`, no errors; the only
  diagnostics are the pre-existing `file-stream.cpp` maybe-uninitialized notes
  and the generated-Core `delete-non-virtual-dtor` warnings
- ROM: `out\rom\helengine_n64_cube_test_rounded_rect.z64`
- SHA-256: `1be5f494ea50cd3c9bf4b4d3f0be3be09cd955569e8dfa1a562659cd1dd42246`
- Bytes: `1589248`, unchanged
- ELF load image: text `982392`, data `235029`, bss `10328`, total `1227749`
  (previous: text `979320`, data `234237`, total `1223885`)

### Which rounded rectangle this scene actually contains

Worth recording, because it is not the one an inspection of the authored
`.helen` suggests. `assets/scenes/rendering/cube_test.helen` carries seven
`RoundedRectComponent`s, including `DemoDiscLightIndicatorSwatch` at (122, 73)
with `Size` 32x32 and `Radius` 2. That entity carries a platform exclusion list
naming `3ds`, `dc`, `ds` and `n64`, so it does not survive the cook: the cooked
N64 scene (`builder\n64-dragonfs\cooked\scenes\rendering\cube_test.hasset`,
7.1 KB) contains exactly one `RoundedRectComponent`, three `SpriteComponent`s
and two `TextComponent`s - the six drawables `n64:scene-ready` reports.

The one rounded rectangle that does ship is
`ConsoleCameraLightInstructionsPanel`, the background of the instruction
overlay: entity position (24, 528), `Size` 345x150, `Radius` 8, `FillColor`
(20, 24, 32, 224), `BorderColor` (120, 140, 170, 255), `BorderThickness` 2,
`RenderOrder2D` 200. The three icon sprites are its children at render order
201, so the panel is drawn immediately before them every frame.

### Boot trace

`out\gopher-rounded-rect-stdout.log`, verbatim:

```
n64:authored-runtime-start
n64:video-ready
n64:renderer-ready
n64:dragonfs-ready
n64:core-ready
n64:startup-scene id=cube_test
n64:texture-loaded id=runtime-texture:cooked/imported/568d5f9ca3b6b3b30b85dfef3ca6746324806b161c8e79298069b4a50878468c size=64x64
n64:texture-loaded id=runtime-texture:cooked/imported/34795cd0703804daa63e3f9eb76b7a644b1baaab10bcaadc6889f6791f257758 size=64x64
n64:texture-loaded id=runtime-texture:cooked/fonts/demodiscbody.hetex size=64x64
n64:texture-loaded id=runtime-texture:cooked/imported/cc1e9bd820dda8c73ad90fde92ee40595d67460755740c344e91130a22940f30 size=64x64
n64:material-loaded id=Materials.rendering.cube_test.CubeTestSolid color=FFFFFFFF lit=1
n64:model-loaded id=engine:model:cube vertices=24 indices=36
n64:scene-ready cameras=1 drawables3d=1 drawables2d=6 directional=1 ambient=0
n64:first-authored-draw model=engine:model:cube material=n64-standard lit=1 lights=1 calls=1
n64:first-rounded-rect-draw size=86x38 radius=200/100 verts=96
n64:first-sprite-draw size=12x12 calls=1
n64:first-frame-complete
```

Every baseline line is unchanged: four `n64:texture-loaded`, the same
`n64:scene-ready` counts, `n64:first-sprite-draw size=12x12 calls=1`,
`n64:first-frame-complete`, and no `n64:error` and no `-skipped` line anywhere
in stdout or stderr. The one new line is the rounded-rect trace, and it reports
the resolve exactly: 345x150 at the viewport's quarter scale is 86.25 by 37.5,
which rounds to 86x38, the radius resolves to 2.00 pixels, and 32 ring segments
produce 96 vertices.

`n64:first-rounded-rect-draw` precedes `n64:first-sprite-draw` because the panel
sits at render order 200 and the icons at 201, which is the ordering this
feature most needed to survive.

### Frame evidence

Overlay-pixel check, counted the same way as the previous entry: pixels more
than 12 in summed channel distance from the clear colour `(16, 16, 24)`, inside
the emulator client area (x 9-633, y 31-479) and outside the cube bounding box
(x 262-381, y 223-319).

| ROM | Overlay pixels | Region |
| --- | --- | --- |
| previous (`_newest_codegen`), booted here for the comparison | 932 | x 23-68, y 365-424 |
| this one | 12282 | x 13-183, y 357-430 |

The previous ROM reproduced its recorded 932 exactly, on two captures, so the
two numbers are comparable. The increase is the panel: 86x38 frame-buffer pixels
upscale to roughly 168x74 in the 640x480 window, and the measured region is 171
by 74 at the lower left, wrapping the icons that used to sit on the clear colour
alone.

This is a change to the recorded baseline figure, and an intended one - a
previously undrawn authored shape is now drawn - but it is not a change to the
icons. Of the 1516 pixels the icons drew over the clear colour in the previous
ROM, 1370 are byte-identical in the new one; the 146 that differ are all edge
pixels of the emulator's upscale filter, each shifted by 4 in green and blue,
exactly the difference between blending an icon edge against `(16, 16, 24)` and
against the panel's `(16, 24, 32)`. Counting pixels far from both the clear
colour and the panel fill - that is, the icon art itself - gives 1122 in both
ROMs.

Cube motion is unaffected. Four captures 400 ms apart differ pairwise by 1186,
776 and 10338 pixels, and every difference bounding box - (262, 223, 380, 319),
(262, 223, 380, 319), (262, 223, 377, 319) - lies inside the cube's.

Read directly, frame 1 shows the cube at roughly 45 degrees with a bright left
face and a dark right side wall, and at the lower left a dark rounded-corner
panel carrying the three icons: a D-pad cross, a round analog stick beside it,
and below them a dark round button with a red glyph. Frame 4 shows the cube
rotated nearly face-on, a flat mid-grey square with no side wall, while the
panel and the three icons are unchanged. A 4x crop of the panel shows the
corners clipped on all four sides, which is the two-pixel radius resolving as
the geometry seam reports it.

### Combiner restore

The restore to `RDPQ_COMBINER_TEX_FLAT` after the triangles is what keeps the
sprites correct, and this scene tests it directly rather than by inspection: the
panel is drawn immediately before the three icon sprites in the same pass, every
frame. Without the restore the icons would have rasterized as flat 12x12 blocks
of the panel's primitive colour, because `RDPQ_COMBINER_FLAT` ignores TEX0. They
render their art, pixel-identical to the previous ROM as counted above.

### Not implemented in this step

`BorderColor` and `BorderThickness` are ignored, so this panel's two-pixel
`(120, 140, 170)` border is absent and the shape is its fill alone. `Color` and
`SourceRect` are ignored. The per-corner `Corners` mask is ignored as well: the
shared `RoundedRectGeometryBuilder.BuildFillRingVertices` takes no per-corner
parameter, so all four corners always round together and an author who selects
only some of them gets all of them. The pass does no blending, so the panel's
authored alpha of 224 reaches the frame buffer opaque - the clear colour behind
it does not show through.

## cube_test with authored text (2026-09-18)

The demodisc `scenes/rendering/cube_test.helen` scene was rebuilt through the
normal editor pipeline with the UI root no longer excluded on N64. This is the
first N64 ROM that draws authored text.

Build workspace
`C:\dev\helworks\builds\helengine-n64\text\work\n64\4abde6e0019d4e8681a02c771767716c`
(`generated-core` and `builder\n64-dragonfs`). Editor log `editor-build2.log`
and native log `native-build3.out.log`, both under
`C:\dev\helworks\builds\helengine-n64\text`. ROM 2031616 bytes, SHA-256
`89692fe9e63879732e144a527b555e036272b8b7ed6e1ad76684c96dd3df56db`.

The editor build fails at its own final step with "An error occurred trying to
start process 'pwsh'". That is the native build executor hardcoding `pwsh`,
which is not installed on this machine; everything before it succeeds. The
native step was rerun by hand with `scripts/build.ps1`, which must itself be
launched through `Start-Process` with redirected streams, because Windows
PowerShell 5.1 turns docker's stderr progress into a terminating error under
`ErrorActionPreference = 'Stop'`.

### What the trace says

```
n64:texture-loaded id=runtime-texture:cooked/fonts/demodiscbody.hetex size=256x256
n64:texture-loaded id=runtime-texture:cooked/fonts/default.hetex size=256x128
n64:scene-ready cameras=1 drawables3d=1 drawables2d=11 directional=1 ambient=0
n64:first-rounded-rect-draw size=86x38 radius=200/100 verts=96
n64:first-sprite-draw size=12x12 calls=1
n64:first-glyph-draw size=8x10 calls=4
n64:command-list commands=87 clip=0/0 quads=3 glyphs=82 rects=2 frame=320x240
n64:first-frame-complete
```

No `n64:error` line appears. The two font atlases arrive at their authored
resolutions rather than the 64 pixels every texture took before, which is the
second cook capability doing its job: the staged filesystem grew from 38 files
and 536741 bytes to 38 files and 906581 bytes, and the whole difference is one
262237-byte atlas and one 131160-byte atlas replacing two 16526-byte ones. The
thirty icon textures are still 16526 bytes each, so the two policies are
genuinely separate rather than the cap having been raised globally.

The panel still fills at 86x38. That number is the evidence that the shared
engine fix reached this ROM: `RenderCommandListBuilder2D.EmitRoundedRect`
previously emitted the authored size unscaled, which would have filled the
panel at 345x150 on a 320-pixel-wide frame buffer.

### Read directly from captures

The FPS overlay renders at the top left across two lines, reading
`Update FPS: 60.0` and `Render FPS: 60.0`. The console instruction panel at the
lower left now carries the labels `Camera` and `Light` beside its three icons,
both legible. The cube continues to rotate: captures taken 900 ms apart show it
at different angles.

### Not implemented, and defects this step found

The millisecond figure in the second FPS row reads `214748364.7 ms`, which is
`int32.MaxValue` divided by ten. Some render-time metric is never fed on this
platform and saturates. The glyphs rendering it are correct; the value is not.

The light-indicator label and swatch render at roughly their authored size
instead of a quarter of it, so the word `Light` fills about a quarter of the
screen. Its subtree hangs under an entity carrying a `ViewportComponent` with a
1280x720 fixed size, and that component is present and registered in the
generated core, so the cause is not that the type was pruned. The console
instruction blueprint in the same frame *is* scaled correctly, so whatever
drives one does not drive the other. Unresolved.

The scene label never appears. It is authored at x=600 inside a 656-wide box on
an unscaled 1280-wide canvas, so its box begins off the right edge of a
320-pixel frame buffer. This was predicted in the design.

Text is bilevel. RGBA 5551 carries one bit of alpha and the pass cuts out at
128, so antialiased glyph edges become hard edges. This is why the first font
scale tried, half the shared value, was rejected: at roughly four pixels tall
the cutout left the rows unreadable. The scale PSP uses is legible.

### One unreproduced hang

The first ROM built in this step crashed once, immediately after
`n64:first-frame-complete`, with `RSP CRASH | rsp_queue |
rspq_syncpoint_wait`, "wait loop timed out (200 ms)", and the RSP halted. That
wait is reached through `rdpq_detach_wait` at the end of the frame. It has not
reproduced in seven subsequent runs across two builds, five of them
back-to-back against the current ROM.

The pass now issues 82 glyph blits per frame where it previously issued three
sprite blits, which is by far the largest change in RDP workload this platform
has seen, and it remains the leading suspect. Against that, the emulator's own
stderr carried repeated `Exhausted LinkedDeviceHost memory` and "will exceed
budget" warnings during the crashing run, so a host-side stall being read as a
200 ms RSP timeout is a live alternative. The command-list trace refuted one
theory outright: there are zero clip commands in the frame, so no degenerate
scissor rectangle is ever issued.
