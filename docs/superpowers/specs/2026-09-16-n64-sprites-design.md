# N64 textures and sprites design (2D step 1 of 3)

Date: 2026-09-16. Repository: helengine-n64 (branch `n64-sprites`). Related
project: demodisc `cube_test` scene. This is the first of three 2D steps
(textures and sprites, then rounded rectangles, then text and the FPS overlay).

## Goal

Render authored `SpriteComponent`s on N64 so the demodisc `cube_test` scene
shows the `ConsoleCameraLightInstructions` overlay with N64 button icons and
its baked-text sprites over the lit, rotating cube in Gopher64.

## Non-goals

- `TextComponent` rendering, fonts, and font-atlas cooking (step 3).
- `RoundedRectComponent` (step 2).
- Input handling; the overlay is display only.
- Sprite rotation and 2D clip rectangles. Rotation is rejected loudly;
  clip rectangles are ignored and documented as a limitation.
- Changing the shared engine or csharpcodegen.

## Codegen profile

`Nintendo64PlatformDefinitionFactory.ForcedDisabledFeatures` is
`debug_overlay;shaders;text2d`, so `render2d` and `sprites` are no longer
pruned. Generated Core then contains the 2D render queue and visitor
interfaces and `SpriteComponent`. Baked text arrives as sprites, so `text2d`
stays disabled in this step.

The profile also declares every `codegen-use-*` option the C++ backend reads,
all true, rather than letting any of them fall through to the CLI preset's
runtime profile. Exceptions and RTTI are load-bearing - the generated Core
throws from its deserializers and selects generic implementations through
`dynamic_cast`, and libdragon compiles it with both enabled - but the remaining
nine are declared for the same reason: an undeclared option follows whichever
preset the generator is invoked with, and the presets available disagree with
what this Core was built from. `docs/RuntimeCompatibility.md` records the set
and the evidence for each value.

## Builder contract

- `Nintendo64PlatformDefinitionFactory` declares one asset cook capability:
  source kind `texture`, target artifact kind `runtime-texture`,
  `PlatformAssetCookOwnershipKind.BuilderOwned`, settings contract id
  `n64-texture`, default settings `MaxResolution = 64`,
  `ColorFormat = Rgba32`, `AlphaPrecision = A8`, format capabilities Rgba32
  with Opaque and A8 alpha.
- `Nintendo64TextureCookSettingsSerializer` serializes and parses
  `TextureAssetProcessorSettings` to and from the settings JSON, mirroring the
  PS2 serializer.
- `Nintendo64SourceTextureDecoder` decodes a cook source into an Rgba32
  `TextureAsset`. A cook source is not always an image file: the editor emits
  some texture sources - the platform font atlases among them - already
  serialized as engine `TextureAsset`s. The decoder chooses by content, reading
  the leading `HELE` magic bytes and deserializing through the shared asset
  serializer when they are present and decoding with System.Drawing when they
  are not. Extension-based detection is not sufficient, because engine-
  serialized sources do not reliably carry a distinguishing one.
- `Nintendo64TextureCooker` box-downscales the decoded texture so its larger
  side is at most `MaxResolution` (never upscales), keeps Rgba32 and A8
  alpha, and sets `Id` and `RuntimeAssetId` from the work item.
- `Nintendo64PlatformCookWorkItemExecutor` runs every
  `Manifest.PlatformCookWorkItems` entry before staging: only source kind
  `texture` is accepted, the cooked `TextureAsset` is written with the shared
  `helengine.files.AssetSerializer` into the package source root at the work
  item's `OutputRelativePath`. Unknown kinds throw
  `InvalidOperationException`.
- The DragonFS stager stages the manifest's cooked artifacts *and* the
  builder's own cook outputs. The editor deliberately keeps builder-owned
  outputs out of the manifest's cooked artifact list, so a stager that follows
  the manifest alone ships a ROM whose sprite textures are absent from its
  filesystem and whose sprites therefore never draw. Builder-cooked outputs are
  staged unconditionally, not filtered against what the scene references.
- `Nintendo64PlatformAssetBuilder.BuildAsync` executes the work items after
  scene validation and before staging.
- `Nintendo64SceneCompatibilityValidator` accepts the runtime feature id
  `texture` in addition to scene, mesh, camera, and material.

## Runtime

Files under `src/platform/n64/`:

- `Nintendo64TexelPacking` (pure, host-testable): converts one RGBA32 row
  into RGBA 5551 words. Alpha at or above 128 sets the alpha bit. Provides
  `PackRgba5551(r, g, b, a)` and `ConvertRow(const uint8_t* rgba, int width,
  uint16_t* output)`.
- `Nintendo64RuntimeTexture` (derives from generated `RuntimeTexture`): owns
  one uncached `surface_t` in `FMT_RGBA16` sized from the cooked asset, filled
  through `Nintendo64TexelPacking`. `Dispose` frees the surface.
- `Nintendo64SpriteGeometry` (pure, host-testable): the whole of the authored
  sprite to blit arithmetic, kept out of the render manager so it can be tested
  without libdragon or a generated Core. `Resolve` takes the texture size, the
  normalized source rect, the authored size and the parent scale as scalars and
  returns a `Nintendo64SpriteBlitGeometry` carrying the texel sub-rect, the
  destination extent, the blit scale factors, and a kind that distinguishes a
  drawable sprite from an empty source and an empty destination. Denormalizing,
  cropping an off-texture sub-rect, the missing-source-rect fallback and the
  degenerate cases all live here; the render manager reads component fields,
  calls `Resolve`, and issues RDP calls.
- `Nintendo64RenderManager2D`:
  - `BuildTextureFromCooked` reads the cooked asset through the content
    stream, requires a `TextureAsset` in Rgba32, and builds the runtime
    texture. `BuildTextureFromRaw` converts a raw Rgba32 asset the same way.
    Other color formats throw.
  - `Draw` runs once per frame after the 3D pass: `rdpq_sync_pipe`,
    `rdpq_set_mode_standard`, alpha compare against 1 for cutout alpha,
    combiner `RDPQ_COMBINER_TEX_FLAT` with the sprite tint in prim color,
    then visits the first camera's 2D queue through `IRenderVisitor2D`,
    calling `drawable->Draw()` so generated Core dispatches back into
    `DrawSprite`.
  - `Draw` is driven by the N64 runtime host, not by `Core::Draw`.
    `Core::Draw` runs the 3D render manager only; `RenderManager2D::Draw` is a
    hook each host calls for itself. The host calls it directly after
    `Core::Draw` and before `EndFrame`, while the frame buffer is still
    attached, because the 2D pass writes to that buffer and depends on the 3D
    pipeline having drained. `Update` and `FlushReleasedTextures` are the other
    two 2D hooks; nothing on this platform needs them yet, so the host does not
    drive them.
  - `DrawSprite` passes the authored `SourceRect`, `Size` and entity scale to
    `Nintendo64SpriteGeometry::Resolve`, skips the sprite when the resolved
    kind is an empty source or an empty destination, and otherwise draws with
    `rdpq_tex_blit` using `s0, t0, width, height, scale_x, scale_y`. Position
    is the entity position in screen pixels. A non-identity entity orientation
    emits `n64:error sprite-rotation` and then throws `NotSupportedException`.
  - `DrawText` and `DrawRoundedRect` remain no-ops in this step.
  - `UpdateTextureRegionCore` remains unsupported.
- Traces: `n64:texture-loaded id=%s size=%ldx%ld` on each texture build and a
  one-time `n64:first-sprite-draw texture=%s size=%ldx%ld` on the first sprite.
- Screen mapping: the overlay's `ViewportComponent` scales its 1280x720
  reference canvas to `RenderManager3D::MainWindowSize`, which the host
  already sets to 320x240 through `AddWindow`.

## Demodisc authoring

- `CubeTestSceneFactory` stops calling `ExcludeN64Root` for the console
  instruction blueprint root. This is the switch that actually puts the
  overlay in the N64 scene. The desktop instruction overlay and the FPS UI root
  stay excluded on N64.
- `ConsoleCameraLightInstructionsAssetCatalog.ConsolePlatformIds` also gains
  `"n64"`, but only for consistency: nothing reads that array, so it selects
  nothing. Do not treat editing it as enabling the overlay on a platform.
- `cube_test.helen` is regenerated through `menu.generate-cube-test-scene`.
- Expected cooked N64 inventory: the scene, the camera, the cube with its spin
  component, the sun, and the instruction root - and with it a long tail of
  sprite payloads. The shipped DragonFS tree holds 38 files: 30 imported
  textures, 4 font atlases, the engine cube model and its material, the
  authored cube material, and the scene. A handful of cooked items is not a
  realistic expectation for this scene, and an inventory check should be
  written against the whole staged tree rather than a short enumerated list.

## Error handling

- Cook failures (missing source, unknown kind, decode failure) fail the build
  with the work item id and path in the message.
- A cooked texture in a color format other than Rgba32 fails at load.
- Sprite rotation fails at draw with `NotSupportedException`, preceded by an
  `n64:error sprite-rotation` trace. Every hard failure raised from inside the
  frame loop emits its `n64:error <reason>` line first: an unhandled native
  exception there stops the ROM with nothing else to explain it.
- Texture allocation failure throws `InvalidOperationException`.

## Testing

- Builder tests: platform definition exposes the `texture` capability with
  the `n64-texture` contract and `MaxResolution = 64`; the codegen profile
  declares the complete `codegen-use-*` set with its values; settings
  serializer round-trips and rejects both an unparseable and a real
  non-Rgba32 format; the cooker downscales a generated 256x256 PNG to 64x64,
  preserves an alpha-zero pixel, never upscales a 16x16 source, rejects
  settings asking for a format it cannot emit, and the executor rejects an
  unknown source kind. Staging is covered with more than one builder-cooked
  work item, so a stager that only ever carried the first one fails.
- Host tests (`scripts/test-host.ps1`): RGBA32 to RGBA5551 packing on known
  texels (pure white, pure channels, alpha 127 versus 128) and one full row;
  and the sprite geometry seam, including denormalization onto real texels, the
  missing-source-rect fallback, clamping past the far edge, cropping rather
  than shifting a sub-rect that starts off the near edge, authored size
  override, parent scale, and both degenerate kinds.
- End-to-end: normal editor build; stdout shows `n64:texture-loaded` lines and
  `n64:first-sprite-draw`; captures show non-background pixels in the overlay
  region that the lit-only ROM does not have, while the cube still rotates.
