# N64 scene lighting design

Date: 2026-09-15. Repository: helengine-n64 (branch `n64-lighting`). Related
project: demodisc `cube_test` scene.

## Goal

Light the existing demodisc `cube_test` cube on N64 from the scene's own
authored lights, using Tiny3D hardware vertex lighting. The scene's
`CubeTestSun` directional light drives the shading. No built-in ambient term is
added: ambient comes only from authored `AmbientLightComponent` instances, so a
scene without one renders back faces black.

## Non-goals

- Point and spot lights. They fail the build with a clear message.
- Textured or transparent materials. The material remains opaque and untextured.
- Shadows, specular, or per-pixel lighting. Tiny3D provides Gouraud vertex
  lighting only.
- Changing the shared engine, csharpcodegen, or other platforms.

## Builder contract

`Nintendo64PlatformDefinitionFactory` and `Nintendo64MaterialCooker` change:

- Material schema id `n64-unlit-v1` is replaced by `n64-standard-v1`
  ("N64 Opaque"). The graphics profile id becomes `n64-standard` and is the
  profile for both build profiles. The runtime family string emitted in the
  cooked asset and logged by the runtime matches.
- The `Lighting` field offers `lit` and `unlit` with default `lit`. The cooker
  sets `PlatformMaterialAsset.Lit` from that field instead of rejecting
  non-unlit values. Texture, transparency, and alpha rules are unchanged.
- `helengine.lightcomponent` becomes `PlatformComponentSupportKind.Transform`.
- `Nintendo64SceneCompatibilityValidator` accepts directional and ambient lights
  and rejects point and spot lights, and rejects more than seven directional
  lights, because Tiny3D supports seven directional lights and one ambient
  color.
- Enforcement note: the build manifest carries no component types, so point
  and spot lights are rejected by editor packaging through the builder's
  Unsupported support rules, and the seven-directional-light limit is checked
  by the runtime host at scene load.

## Demodisc authoring

`CubeTestSceneFactory` stops calling `ExcludeN64Root` for the directional light
entity. The scene is regenerated through the existing
`menu.generate-cube-test-scene` editor command, so the cooked N64 inventory
contains the camera, the cube mesh with `CubeTestSpinComponent`, and
`CubeTestSun`. No ambient light is authored in this scene.

## Runtime

Files under `src/platform/n64/`:

- `Nintendo64RuntimeModel` packs each cooked normal with
  `t3d_vert_pack_normal`. A model whose cooked asset has no normals, or whose
  normal count differs from the position count, records that it is unlit-only.
  Drawing such a model with a lit material throws
  `InvalidOperationException` rather than rendering black.
- `Nintendo64RuntimeMaterial` carries the `Lit` flag from the cooked asset.
- A new `Nintendo64FrameLights` value type holds the ambient color and up to
  seven directional entries (8-bit RGB color and a unit `T3DVec3`). A new
  `Nintendo64LightExtractor` builds it from the generated `ObjectManager`
  lists each frame: ambient is the intensity-scaled sum of ambient lights
  clamped to 255, or black when none exist; each directional entry uses
  `LightDirectionUtility::GetLightDirection` for the direction and
  `Color * Intensity` clamped for the color. Tiny3D expects the vector that
  points from the surface toward the light, so the entity forward direction is
  negated. The sign is verified visually during validation.
- `Nintendo64RenderManager3D::Draw` extracts the frame lights once per frame
  and passes them with the material's `Lit` flag to
  `Nintendo64Renderer::DrawAuthoredMesh`.
- `Nintendo64Renderer::DrawAuthoredMesh` for lit draws calls
  `t3d_light_set_ambient`, `t3d_light_set_directional` per entry,
  `t3d_light_set_count`, and uses draw flags without `T3D_FLAG_NO_LIGHT`.
  Unlit draws keep the existing path. The diagnostic cube stays unlit.
- The first authored draw trace gains the light count:
  `n64:first-authored-draw model=... material=n64-standard lit=1 lights=1`.

## Error handling

- Unsupported light types and more than seven directional lights fail at
  package validation, not at runtime.
- A lit material on a model without normals fails at the first draw with an
  exception, preserving the existing fail-loud policy.
- Colors are clamped to the 0-255 range; direction vectors are normalized and
  a zero-length direction throws.

## Testing

- Builder tests: cooker emits `Lit=true` by default and `Lit=false` for
  `unlit`; validator accepts directional plus ambient and rejects a point
  light; platform definition exposes the `lit`/`unlit` choice and the
  `n64-standard` profile.
- Host tests via `scripts/test-host.ps1`: light extraction math (color
  scaling and clamping, ambient summation, direction negation and
  normalization) and normal packing against known values, using the existing
  stub layout.
- End-to-end: normal editor build of demodisc for N64, confirm the cooked
  inventory includes `CubeTestSun`, boot the ROM in Gopher64, and capture
  frames. Success means the visible cube faces have distinct brightness that
  changes as the cube rotates; uniform white faces mean lighting is not
  applied.
