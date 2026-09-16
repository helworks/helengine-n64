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
