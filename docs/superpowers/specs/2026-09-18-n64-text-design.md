# N64 text rendering and the FPS overlay (2D step 3 of 3)

Date: 2026-09-18. Repository: helengine-n64. Related project: demodisc
`cube_test` scene. This is the last of three 2D steps (textures and sprites,
then rounded rectangles, then text and the FPS overlay).

## Goal

Render authored `TextComponent`s on N64 so the demodisc `cube_test` scene shows
the FPS counter, the scene label and the light-indicator label over the lit,
rotating cube in Gopher64.

## Non-goals

- Input. The two input-driven components on the UI root stay excluded on N64.
- Text selection, the caret, and hit testing. `TextComponent` carries all of it
  and none of it is driven here.
- Blending. The 2D pass still writes opaque texels through an alpha-compare
  cutout, so authored alpha below the cutout disappears rather than blending.
- Changing the shared engine, the shared font charset, or csharpcodegen.

## What is already in the ROM

Three findings set the scope of this step, and two of them make it smaller than
the previous two steps rather than larger.

**The text machinery is already compiled in.** The current build workspace's
`generated-core` contains `TextComponent`, `ITextDrawable2D`, `FPSComponent`,
`FontAsset`, `FontChar`, `RenderCommandList2D`, `RenderCommandListBuilder2D`,
`TextLayoutUtils`, `TextLayoutAlignmentUtils`, `TextRenderEffectPassBuilder`
and a registered `GeneratedRuntimeTextComponentDeserializer`, and
`generated_unity.cpp` includes them. This is true even though
`Nintendo64PlatformDefinitionFactory.ForcedDisabledFeatures` still lists
`text2d`. Feature pruning drops only the types named in the feature catalog's
root rules, and the reachability planner puts a pruned type back whenever a
surviving type references it. `FPSComponent` is not gated by any feature and
references `TextComponent`, so the gate never closes. **No codegen profile
change is required by this step**, and the ROM is already paying for the code.

The stale `text2d` entry should be dropped from `ForcedDisabledFeatures`
because it does not do what its comment claims, not because removing it changes
the output. Removing it is expected to produce a byte-identical generated Core,
and that expectation is the test.

**Glyph layout is shared C# that already runs here.**
`RenderCommandListBuilder2D.Build(IRenderQueue2D)` turns one camera's 2D queue
into a flat `RenderCommandList2D`: a sequence of `ClipPush`, `ClipPop`,
`TexturedQuad`, `GlyphQuad` and `RoundedRect` commands. Wrapping, per-line
horizontal alignment, line breaking, and the per-glyph atlas source rect and
destination pixel bounds are all resolved inside the builder. Outline and
shadow are not effects: `TextRenderEffectPassBuilder` expands them into
additional `GlyphQuad` commands, one full copy of every glyph per pass, with
the main pass emitted last. A platform therefore needs no layout code at all,
and a glyph quad draws through exactly the same textured blit as a sprite quad.

**The font atlas cook is broken for fonts.** Every one of the 38 files in the
staged DragonFS tree is 16526 bytes, which is a 64x64 RGBA32 payload, because
N64 registers exactly one cook capability, for source kind `texture`, with
`MaxResolution = 64`. The editor's `EditorPlatformCookWorkItemFactory` looks
for a builder-owned capability for source kind `font-atlas-texture` and falls
back to `texture` when a platform has none, so font atlases take the 64-pixel
cap. The editor default atlas is 256x128 and the demodisc body atlas is
256x256, so they currently ship at 64x32 and 64x64 and their glyphs land at two
or three pixels. PlayStation 2 and PSP both register a distinct
`font-atlas-texture` capability, so the lever is precedented and entirely
platform-local. The shared font charset and atlas generation are not touched.

## Architecture

The N64 2D pass moves from the drawable visitor to the shared command list.

Today `Nintendo64RenderManager2D::Draw` visits the first camera's queue through
`IRenderVisitor2D` and lets generated Core dispatch back into `DrawSprite` and
`DrawRoundedRect`. That path cannot render text without reimplementing
wrapping, alignment and the effect passes in C++. Instead `Draw` retains one
`RenderCommandListBuilder2D`, calls `Build` on the camera's queue, and executes
the returned command list. Sprites, glyphs and rounded rectangles then share
one executor, clip rectangles stop being an ignored limitation, and the layout
logic stays in the shared engine where every other platform reads it. The Wii
consumes the same list the same way, so the call shape is proven rather than
invented.

The cost is that two working draw paths are rewritten. The mitigation is that
the arithmetic seams survive: the executor still resolves geometry through pure
host-tested code, and the rounded-rect seam is reused unchanged.

## Runtime

Files under `src/platform/n64/`:

- `Nintendo64QuadBlitGeometry` (new, pure, host-testable). Resolves one
  textured quad command. Given the destination bounds in pixels, the normalized
  source rect, and the atlas or texture size in texels, it returns the texel
  sub-rect, the integer destination origin and extent, and the two blit scale
  factors, plus a kind distinguishing a drawable quad from an empty source and
  an empty destination. Sprite quads and glyph quads both resolve through it,
  because the command list has already reduced both to the same shape.
  `Nintendo64SpriteGeometry` resolved from authored size and parent scale; the
  command list does that part now, so this seam is the narrower successor and
  the old one is removed with its tests folded into the new suite.
- `Nintendo64ClipRectStack` (new, pure, host-testable). Resolves `ClipPush` and
  `ClipPop` into scissor rectangles: intersects each pushed rect with the
  enclosing one and with the frame, reports an empty result rather than a
  negative extent, and restores the enclosing rect on pop. Keeping the stack
  out of the executor is what makes nesting and the empty case testable without
  libdragon.
- `Nintendo64RenderCommandExecutor2D` (new). Walks one `RenderCommandList2D`
  and issues RDP calls: `rdpq_tex_blit` for textured and glyph quads,
  `rdpq_triangle` with `TRIFMT_FILL` for rounded-rect fills, and
  `rdpq_set_scissor` for clip commands. It owns the combiner discipline that
  the rounded-rect step established: the fill switches to a flat combiner and
  restores the textured one before returning, so a glyph drawn immediately
  after a panel is unaffected. This is a separate class from the render manager
  because the manager is already 15.6K and the executor is the part that needs
  its own reading.
- `Nintendo64RenderManager2D` (changed). Retains one
  `::RenderCommandListBuilder2D*` for the run, builds one list per enabled
  camera per frame, and hands each to the executor. `DrawSprite`, `DrawText`
  and `DrawRoundedRect` become fail-loud overrides that emit `n64:error
  unexpected-drawable-dispatch` and throw `NotSupportedException`: generated
  Core must not reach them once the builder is the visitor, and a silent no-op
  there would be the same class of defect as the dead 2D pass shipped in the
  sprites step.
- `Nintendo64RuntimeTexture` is unchanged. A font atlas is an ordinary cooked
  texture and reaches RGBA 5551 through the existing packing path.

Traces: `n64:first-glyph-draw font=%s size=%ldx%ld` once on the first glyph
quad, and `n64:command-list camera=%ld commands=%ld` once per run so an empty
list is distinguishable from an unexecuted one.

## Text quality

RGBA 5551 carries one bit of alpha, and the pass cuts out at alpha 128, so
antialiased glyph edges become hard edges. Text renders bilevel. At the sizes
this scene uses that is legible and is what the hardware's contemporaries did,
but it is a real fidelity gap and it is the first thing to revisit. The better
answer is `FMT_IA8`, which is one byte per texel rather than two and carries
four bits of alpha, so it would both halve atlas memory and restore gradation.
That is a new texel format path and a new cooked color format, so it is
deliberately out of this step.

Atlas memory at `MaxResolution = 256` is 64 KB for a 256x128 atlas and 128 KB
for a 256x256 one, against 4 MB of system RAM. Only atlases referenced by a
deserialized `FontAsset` are built.

## Builder contract

- `Nintendo64PlatformDefinitionFactory` registers a second
  `PlatformAssetCookCapabilityDefinition`: source kind `font-atlas-texture`,
  target artifact kind `runtime-texture`,
  `PlatformAssetCookOwnershipKind.BuilderOwned`, settings contract id
  `n64-font-atlas-texture`, default settings `MaxResolution = 256`,
  `ColorFormat = Rgba32`, `AlphaPrecision = A8`. The existing `texture`
  capability keeps `MaxResolution = 64`, so icons do not grow.
- `ForcedDisabledFeatures` becomes `debug_overlay;shaders`.
- Component support rules gain `helengine.TextComponent` and
  `helengine.FPSComponent` as `PlatformComponentSupportKind.Transform`.
- `Nintendo64PlatformCookWorkItemExecutor` accepts source kind
  `font-atlas-texture` in addition to `texture`, resolving the settings from
  the matching capability rather than assuming one contract. An unknown kind
  still throws with the work item id and path in the message.
- `Nintendo64SceneCompatibilityValidator.SupportedFeatures` gains `text` and
  `font`.

## Demodisc authoring

- `CubeTestSceneFactory` gains a third exclusion tier. The UI root moves off
  `MinimalRuntimeExcludedPlatformIds` onto a new
  `TextlessRuntimeExcludedPlatformIds = [dc]`, which is what actually puts the
  FPS counter, the scene label and the light indicator into the N64 scene. The
  desktop instruction overlay stays on the minimal tier, and the console
  instruction blueprint stays on the spriteless tier.
- The two input-driven components on the UI root,
  `DemoDiscReturnToMenuComponent` and `DemoDiscLightToggleComponent`, are
  excluded per component on N64 through the same
  `PlatformSceneAuthoringHelperService` path `ExcludeN64OrbitComponent`
  already uses. Excluding a component is not the same as excluding its root,
  and the FPS counter lives on the same entity as both of them.
- `Nintendo64FpsComponentOverrideService` mirrors
  `PspFpsComponentOverrideService` exactly: it persists an N64-only
  `FPSComponent.FontScale` through `ComponentPlatformEditingService` without
  excluding anything. PSP halves the shared scale of 2 to 1 for a 480x272
  screen; N64 is 320x240 and needs its own value. The starting value is 0.5 and
  it is tuned against a capture rather than argued from arithmetic, because
  what matters is the rendered pixel height of the atlas's line height at that
  scale.
- The scene label needs an N64 transform override. It is authored at x=600 in
  an unscaled 1280-wide space inside a 656-wide box, and the light indicator
  overlay has its own viewport entity with a 1280x720 reference canvas while
  the label does not. On a 320-pixel framebuffer the label's box therefore
  starts off the right edge entirely.
  `EntitySaveComponent.GetOrCreateTransformPlatformOverride` persists a
  per-platform transform, and the label's `Size` takes a component property
  override the same way the FPS scale does. Both values are tuned against a
  capture. If the label cannot be made to land legibly, it is excluded on N64
  and that is recorded, rather than shipped off-screen.
- `cube_test.helen` is regenerated through `menu.generate-cube-test-scene`.

## Error handling

- A `GlyphQuad` whose texture is null, or whose `FontAsset` never resolved an
  atlas, emits `n64:error glyph-missing-atlas` and throws. A font that silently
  draws nothing is the failure mode this step exists to avoid.
- A cooked atlas in a color format other than Rgba32 fails at load, as ordinary
  textures already do.
- Clip rectangles that resolve empty skip their contents; they are not an
  error.
- Every hard failure raised inside the frame loop emits its `n64:error
  <reason>` line first. An unhandled native exception there stops the ROM with
  nothing else to explain it.

## Testing

- Host tests (`scripts/test-host.ps1`): the quad-blit seam, including
  denormalizing a glyph sub-rect onto real texels, a sub-rect that starts off
  the near edge cropping rather than shifting, non-uniform destination scale on
  each axis independently so an X/Y swap fails, and both degenerate kinds; and
  the clip stack, including nesting, an inner rect larger than its parent
  clamping to the parent, a disjoint inner rect resolving empty, and pop
  restoring the enclosing rect.
- Builder tests: the platform definition exposes both cook capabilities with
  their distinct contract ids and resolutions, and a test pins that the font
  capability's 256 and the texture capability's 64 are not the same value; the
  cooker leaves a 256x128 atlas unscaled under the font settings while the same
  atlas under the texture settings becomes 64x32; the work item executor routes
  each source kind to its own settings and still rejects an unknown kind.
- Codegen: removing `text2d` from `ForcedDisabledFeatures` produces a generated
  Core byte-identical to the current one. If it does not, the finding above is
  wrong and the step stops to re-examine it.
- End-to-end: normal editor build, then captures. The FPS counter must show
  digits that change between captures taken seconds apart, which is the only
  check that distinguishes live text from a static blit. The scene label and
  the indicator label must be readable. The cube must still rotate and the
  icons and panel must be unchanged from the rounded-rect baseline except where
  the new text overlaps them.

## Known limitations recorded by this step

Text is bilevel because the pass has one bit of alpha and no blending. Outline
and shadow work, because they are ordinary glyph quads, but each costs a full
extra copy of every glyph. The rounded-rect border and the per-corner mask stay
unimplemented from the previous step.
