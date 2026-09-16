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

`Nintendo64PlatformDefinitionFactory.ForcedDisabledFeatures` becomes
`debug_overlay;shaders;text2d`, so `render2d` and `sprites` are no longer
pruned. Generated Core then contains the 2D draw pass in `Core::Draw`, the 2D
render queue and visitor interfaces, and `SpriteComponent`. Baked text arrives
as sprites, so `text2d` stays disabled in this step.

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
- `Nintendo64SourceTextureDecoder` decodes a source PNG into an Rgba32
  `TextureAsset` with System.Drawing.
- `Nintendo64TextureCooker` box-downscales the decoded texture so its larger
  side is at most `MaxResolution` (never upscales), keeps Rgba32 and A8
  alpha, and sets `Id` and `RuntimeAssetId` from the work item.
- `Nintendo64PlatformCookWorkItemExecutor` runs every
  `Manifest.PlatformCookWorkItems` entry before staging: only source kind
  `texture` is accepted, the cooked `TextureAsset` is written with the shared
  `helengine.files.AssetSerializer` into the package source root at the work
  item's `OutputRelativePath`, so the existing DragonFS stager copies it.
  Unknown kinds throw `InvalidOperationException`.
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
  - `DrawSprite` derives the source sub-rectangle from `SourceRect` times the
    texture size, the destination size from `Size` (or the texture size when
    zero) times the entity scale, and draws with `rdpq_tex_blit` using
    `s0, t0, width, height, scale_x, scale_y`. Position is the entity
    position in screen pixels. A non-identity entity orientation throws
    `NotSupportedException`.
  - `DrawText` and `DrawRoundedRect` remain no-ops in this step.
  - `UpdateTextureRegionCore` remains unsupported.
- Traces: `n64:texture-loaded id=%s size=%ldx%ld` on each texture build and a
  one-time `n64:first-sprite-draw texture=%s size=%ldx%ld` on the first sprite.
- Screen mapping: the overlay's `ViewportComponent` scales its 1280x720
  reference canvas to `RenderManager3D::MainWindowSize`, which the host
  already sets to 320x240 through `AddWindow`.

## Demodisc authoring

- `ConsoleCameraLightInstructionsAssetCatalog.ConsolePlatformIds` gains
  `"n64"`. The blueprint is regenerated through its existing editor command.
- `CubeTestSceneFactory` stops calling `ExcludeN64Root` for the console
  instruction blueprint root. The desktop instruction overlay and the FPS UI
  root stay excluded on N64.
- `cube_test.helen` is regenerated through `menu.generate-cube-test-scene`.
- Expected cooked N64 inventory: camera, cube with spin component, sun, and
  the instruction root with sprite components whose textures resolve to cooked
  N64 textures.

## Error handling

- Cook failures (missing source, unknown kind, decode failure) fail the build
  with the work item id and path in the message.
- A cooked texture in a color format other than Rgba32 fails at load.
- Sprite rotation fails at draw with `NotSupportedException`.
- Texture allocation failure throws `InvalidOperationException`.

## Testing

- Builder tests: platform definition exposes the `texture` capability with
  the `n64-texture` contract and `MaxResolution = 64`; settings serializer
  round-trips; the cooker downscales a generated 256x256 PNG to 64x64,
  preserves an alpha-zero pixel, never upscales a 16x16 source, and the
  executor rejects an unknown source kind.
- Host test (`scripts/test-host.ps1`): RGBA32 to RGBA5551 packing on known
  texels (pure white, pure channels, alpha 127 versus 128) and one full row.
- End-to-end: normal editor build; stdout shows `n64:texture-loaded` lines and
  `n64:first-sprite-draw`; captures show non-background pixels in the overlay
  region that the lit-only ROM does not have, while the cube still rotates.
