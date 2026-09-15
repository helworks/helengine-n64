# Authored N64 scene build

The authored mode is selected with `-Mode authored` and requires a generated
Core tree. It refuses to fall back to the diagnostic cube when `Core.hpp` is
missing. Diagnostic mode remains available with `-Mode diagnostic`.

The generic editor/build graph supplies the generated Core and cooked manifest.
The N64 builder stages only manifest-declared scene/material/model payloads,
writes `n64_runtime_scene_catalog.hpp/.cpp`, and invokes the existing Docker
toolchain. The runtime initializes DragonFS, loads the startup scene through
`Core::SceneManager`, and sends its camera and drawable submissions through the
Tiny3D renderer.

The current runtime supports one perspective camera, one static triangle-list
mesh, and one opaque unlit material. Authored world position, scale, rotation,
camera transform/projection, and base color are read from the generated runtime
objects. The old hardcoded cube is only used in diagnostic mode.

## Reproducible fixture

The checked-in generator at `tools/helengine.n64.samplegen` creates the camera,
mesh, and material through the editor `AssetSerializer` and the generic N64
platform packager. Both authored entities explicitly use render layer `1` so
the mesh passes the camera queue's layer-mask filter. The generated runtime
scene loader preserves that serialized mask.

Generate a package and stage it for the N64 build:

```powershell
pwsh -NoProfile -File scripts/generate-authored-fixture.ps1 `
  -ProjectRoot C:\dev\helworks\builds\helengine-n64\v02\fixture-project `
  -PackageRoot C:\dev\helworks\builds\helengine-n64\v02\fixture-package
New-Item -ItemType Directory -Force C:\dev\helworks\builds\helengine-n64\v02\filesystem-authored\cooked | Out-Null
Copy-Item C:\dev\helworks\builds\helengine-n64\v02\fixture-package\cooked\* `
  C:\dev\helworks\builds\helengine-n64\v02\filesystem-authored\cooked -Recurse -Force
```

Build with the final generated Core tree and the staged cooked assets:

```powershell
pwsh -NoProfile -File scripts/build.ps1 -Clean -Mode authored `
  -GeneratedCoreRoot C:\dev\helworks\builds\helengine-n64\v02\generated\core-authored2 `
  -FilesystemRoot C:\dev\helworks\builds\helengine-n64\v02\filesystem-authored
```

Build outputs and generated sources for the v0.2 checkpoint are kept under
`C:\dev\helworks\builds\helengine-n64\v02`.

## Runtime evidence

The target-side authored startup trace reaches `first-authored-draw` with
`engine:model:cube` and `first-frame-complete`. The Gopher64 capture showed the
authored white cube face against the configured dark clear color. Hardware
execution remains unverified.
