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

## Existing demodisc cube_test

The approved N64 scene build uses the existing authored source at
`C:\dev\helprojs\demodisc\assets\scenes\rendering\cube_test.helen` and its
original `city.rendering.CubeTestSpinComponent`.

Regenerate that scene through the editor before running the normal N64 build:

```powershell
dotnet C:\dev\helworks\helengine\helengine.ui\helengine.editor.app\bin\Debug\net9.0-windows\helengine.editor.app.dll `
  --project C:\dev\helprojs\demodisc\project.heproj `
  --editor-command menu.generate-cube-test-scene
```

The demodisc N64 build entry selects `cube_test`, runs the existing generation
command as its release prebuild, and uses the N64 unlit/DragonFS/cartridge
profiles. The authored N64 overrides retain the camera, cube mesh, and original
spin component while excluding unsupported instruction/UI/light and orbit
content. The N64 camera adapter uses the engine's canonical authored `-Z`
forward axis; the Tiny3D SRT call continues to receive the authored quaternion.
See `C:\dev\helworks\builds\helengine-n64\cube-test\post-override-inventory.md`
for the cooked inventory and focused override regression result.

The current editor build configuration persists selected scenes as canonical
`selectedSceneReferences`; the legacy `selectedSceneIds` JSON property is
ignored by the normal queue loader. The demodisc `n64` entry therefore carries
one file-backed reference for `scenes/rendering/cube_test.helen`. Verify the
resolved queue contains only `cube_test` before a build so unrelated authored
scenes do not enter the N64 cook.

The minimal N64 codegen profile leaves the enabled-feature option unset, which
means no catalog feature is enabled by default. Its forced-disabled list uses
only feature ids declared by the checked-in HelEngine catalog.

The editor copies the final package tree into the builder working directory's
`package-source` subdirectory before invoking the platform builder. N64
DragonFS staging reads manifest-declared `cooked/...` paths from that package
source root; the sibling workspace `cooked` directory remains the editor's
cook output and is not the builder input root.

The registered native executor passes `-Clean` to the Docker build wrapper so
dependency files from an earlier generated Core profile cannot affect the
authored unity compilation. The executor launches `pwsh`, so PowerShell 7 must
be on the editor process PATH; when it is not, generation and staging still
complete and the native step can be rerun with an explicit `pwsh.exe` path
against the finalized `generated-core` and `builder\n64-dragonfs` directories
of the newest workspace under `HELENGINE_BUILD_WORKSPACE_ROOT\n64`.

After changing the N64 builder source, rebuild the registered builder assembly
without rebuilding its already available HelEngine references:

```powershell
dotnet build C:\dev\helworks\helengine-n64-impl\builder\helengine.n64.builder.csproj `
  --no-restore `
  -p:HelengineRoot=C:\dev\helworks\helengine `
  -p:BuildProjectReferences=false
```

Register the local catalog, set both required environment variables, and then
run the normal project build. The registration updates only `n64` and validates
every path; the environment variables keep the native checkout and isolated
build graph under the visible cube-test output directory:

```powershell
pwsh -NoProfile -File C:\dev\helworks\helengine-n64-impl\scripts\register-development-platform.ps1 `
  -EngineUserSettingsRoot C:\dev\helworks\helengine\user_settings `
  -EngineVersion 1.0.0+ec2d7ff8a5b2624bbc51784bd9eb0c9babed1843 `
  -N64RepositoryRoot C:\dev\helworks\helengine-n64-impl `
  -BuilderAssemblyPath C:\dev\helworks\helengine-n64-impl\builder\bin\Debug\net9.0\helengine.n64.builder.dll `
  -GeneratedCoreCppRoot C:\dev\helworks\builds\helengine-n64\cube-test\generated\core `
  -CodegenToolPath C:\dev\helworks\csharpcodegen\codegen\bin\Release\net9.0\codegen.exe

$env:HELENGINE_N64_REPOSITORY_ROOT = 'C:\dev\helworks\helengine-n64-impl'
$env:HELENGINE_BUILD_WORKSPACE_ROOT = 'C:\dev\helworks\builds\helengine-n64\cube-test\work'

dotnet C:\dev\helworks\helengine\helengine.ui\helengine.editor.app\bin\Debug\net9.0-windows\helengine.editor.app.dll `
  --project C:\dev\helprojs\demodisc\project.heproj `
  --build n64 --build-profile release `
  --output C:\dev\helworks\builds\helengine-n64\cube-test\editor-output
```
