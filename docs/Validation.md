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
