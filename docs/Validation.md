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
