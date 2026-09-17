# Authored N64 scene build

The authored mode is selected with `-Mode authored` and requires a generated
Core tree. It refuses to fall back to the diagnostic cube when `Core.hpp` is
missing. Diagnostic mode remains available with `-Mode diagnostic`.

## Toolchain prerequisite

N64 requires a C++ code generator built from csharpcodegen commit `b477b70` or
later, which is the first to emit `runtime/native_runtime.hpp`. The shared
`csharpcodegen\codegen\bin\Release` executable satisfies that: it has been
rebuilt from csharpcodegen `master` `c3c0521` and carries both
`runtime/native_runtime.hpp` and `runtime/native_endian.hpp`. No privately built
generator is needed - the `n64` platform row points at the shared executable,
and `docs/Validation.md` records a full regenerate, rebuild and boot against it.
If a native compile does fail with `'HeCppString' does not name a type`, the
shared build has gone stale again; check that before reading the failure as a
problem with the authored scene.

The editor the recorded ROM was generated with also carried helengine commit
`c0157e99`, the null-guard on the draw-timing path. That commit has since landed
on helengine `main`, so regenerating no longer depends on building from a feature
branch.

This factory declares `PlatformSerializationEndianness.LittleEndian`, which
reaches the generated config as `HE_CPP_PLATFORM_IS_LITTLE_ENDIAN 1`. That value
describes the byte order of cooked data, not the processor, so a big-endian
console reading little-endian payloads is a consistent configuration rather than
a defect. csharpcodegen `d8ad75d` makes that explicit: `runtime/native_endian.hpp`
derives the host order from the compiler alone and no longer ties it to the
declared serialization order.

Cooked assets are read through the engine's `EngineBinaryReader`, which dispatches
to `BinaryReaderBE` or `BinaryReaderLE`; those assemble each value byte by byte
and are endian-correct on any host by construction. That is why this platform
loads and renders every cooked asset correctly despite running big-endian
hardware against little-endian payloads.

What the declaration does cost is a byte swap on every multi-byte field this
console reads, where GameCube and Wii declare `BigEndian` and pay nothing.
Changing it would require re-cooking assets, so it is a deliberate performance
question rather than a correction, and nothing forces it.

The generic editor/build graph supplies the generated Core and cooked manifest.
The N64 builder stages only manifest-declared scene/material/model payloads,
writes `n64_runtime_scene_catalog.hpp/.cpp`, and invokes the existing Docker
toolchain. The runtime initializes DragonFS, loads the startup scene through
`Core::SceneManager`, and sends its camera and drawable submissions through the
Tiny3D renderer.

The current runtime supports one perspective camera, one static triangle-list
mesh, and one opaque material drawn through the `n64-standard` graphics profile
and the `n64-standard-v1` material schema. That schema's `Lighting` choice field
(`lighting-mode`) defaults to `lit`; `unlit` must be authored explicitly. Lit
materials carry packed per-vertex normals and are drawn with Tiny3D vertex
lighting.

Scene lights come exclusively from authored `DirectionalLightComponent` and
`AmbientLightComponent` entities: the runtime extracts them each frame and
applies no built-in ambient term, so a lit scene with no authored light renders
black. At most seven directional lights are accepted; point and spot lights are
rejected by editor packaging.

Authored world position, scale, rotation, camera transform/projection, and base
color are read from the generated runtime objects. The old hardcoded cube is
only used in diagnostic mode and is drawn unlit.

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
command as its release prebuild, and uses the `n64-standard`/DragonFS/cartridge
profiles. The authored N64 overrides retain the camera, cube mesh, original spin
component, and the `CubeTestSun` directional light, while excluding unsupported
instruction/UI and orbit content. The N64 camera adapter uses the engine's
canonical authored `-Z` forward axis; the Tiny3D SRT call continues to receive the authored quaternion.
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

The N64 codegen profile no longer forces `render2d` and `sprites` off, so a
scene that carries authored `SpriteComponent`s auto-detects both features and
the runtime renders them in a screen-space 2D pass after the Tiny3D 3D pass.
`text2d` stays forced off, so authored `TextComponent`s are still not drawn.

Authored `RoundedRectComponent`s now have their fill drawn. The screen-space
resolve lives in `Nintendo64RoundedRectGeometry`, a pure host-tested seam that
multiplies the authored size by the parent scale on each axis, scales the radius
by the smaller of the two factors and clamps it to half the smaller resolved
side. The ring itself is not reimplemented here: `DrawRoundedRect` calls the
generated Core's `RoundedRectGeometryBuilder::BuildFillRingVertices`, which
returns 32 segments as centre/boundary/boundary triples, and hands each triple
to `rdpq_triangle` with the predefined `TRIFMT_FILL` format. `TRIFMT_FILL`
carries position only, so the triangles run under `RDPQ_COMBINER_FLAT` with the
shape's `FillColor` as the primitive colour; the pass otherwise leaves the
combiner on `RDPQ_COMBINER_TEX_FLAT` for sprites, and `DrawRoundedRect` restores
it before returning because the ordered queue can put a sprite straight after a
rectangle.

Fill is all that is drawn. `BorderColor` and `BorderThickness` are ignored, so a
bordered shape renders as its fill alone, and the texture-modulation fields
`Color` and `SourceRect` are ignored too - the fill is flat, never a tinted
nine-slice. A non-zero `Rotation` is refused rather than drawn unrotated: the
runtime traces `n64:error rounded-rect-rotation` and throws
`NotSupportedException`. There is no blending in this pass, so a `FillColor`
with a partial alpha draws fully opaque; `cube_test`'s instruction panel is
authored at alpha 224 and reaches the frame buffer solid.

An instruction overlay therefore reaches an N64 ROM as its panel and its icons,
without its labels.

`Core::Draw` only drives the 3D render manager. `RenderManager2D::Draw` is a
hook each host calls for itself, so the N64 runtime host runs the 2D pass right
after `Core::Draw` and before `EndFrame`, while the frame buffer is still
attached.

`Draw` is the only 2D hook the N64 host currently drives. The generated
`RenderManager2D` also declares `Update()` and `FlushReleasedTextures()`, and
neither is called from the N64 frame loop. Nothing on this platform needs them
yet - sprite textures are built once from cooked payloads and never released or
re-uploaded mid-frame - but a feature that relies on either one (deferred
texture release, animated or streamed 2D content) will need the host to start
driving them first.

The N64 codegen profile declares all eleven `codegen-use-*` options the C++
backend reads, every one of them true. Exceptions and RTTI are the two the Core
visibly breaks without - it throws from generated deserializers and selects
generic implementations through `dynamic_cast` - but any option left undeclared
inherits whichever runtime profile the CLI preset carries, and both the
`native-core-boot` and `n64-minimal` presets carry one that would regenerate a
Core this repository cannot compile. The full set with its evidence is in
`docs/RuntimeCompatibility.md`.

The builder owns texture cooking for N64. Each declared cook work item is
decoded (authored image files through the system decoder, editor-generated
engine texture assets such as the platform font atlases through the asset
serializer, chosen by the file's `HELE` magic bytes), box-filtered down to at
most 64 pixels on its larger side, and written into `package-source` as
straight RGBA32. The editor deliberately leaves builder-owned outputs out of
the manifest's cooked artifacts, so staging copies the manifest artifacts and
the builder's own cook outputs into the `n64-dragonfs` staging tree; staging the
manifest alone would ship a ROM whose sprite textures are missing.

The registered native executor passes `-Clean` to the Docker build wrapper so
dependency files from an earlier generated Core profile cannot affect the
authored unity compilation. The executor launches `pwsh`, so PowerShell 7 must
be on the editor process PATH; when it is not, generation and staging still
complete and the native step can be rerun with an explicit `pwsh.exe` path
against the finalized `generated-core` and `builder\n64-dragonfs` directories
of the workspace the build just produced.

Do not pick that workspace by newest write time alone. A build can leave a
second, later directory holding only generated-dotnet, so the newest entry is
not always the one that was built. Select the newest directory that actually
contains a generated-core subdirectory, and confirm the staged filesystem
directory is present beside it before using it.

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
