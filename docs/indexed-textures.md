# Indexed textures on Nintendo 64

The builder accepts generic `Rgba32`, `Indexed4` and `Indexed8` assets. Indexed
cooking uses the selected `QuantizedIndexed` strategy after the existing private
box-filter resize. Image defaults remain `Rgba32/A8` with a resolution cap of 64.
Font-atlas defaults are `Indexed8/A8`, `QuantizedIndexed`, with a cap of 512.

The serialized indexed payload keeps the shared engine contract: RGBA byte
palettes, one byte per Indexed8 texel, and low-nibble-first Indexed4 texels packed
across the whole image. The native runtime uploads Indexed8 directly to `FMT_CI8`
and repacks Indexed4 into high-nibble-first, byte-aligned `FMT_CI4` rows.
Neither indexed upload expands the image into RGBA texels.

Grayscale palettes use an IA16 TLUT, preserving eight-bit glyph coverage.
Color palettes use an RGBA5551 TLUT, matching the previous direct texture color
precision. Both index sizes validate their payload and palette references before
replacing an existing texture. Texels and palette use owned uncached memory and
outlive queued RSP/RDP commands.

Libdragon splits large scaled and rotated CI blits into the lower 2048 bytes of
TMEM; the upper half contains the palette. A cache local to each executor call
reuses consecutive bindings of the same palette. Switching indexed textures
reloads the TLUT, and a direct RGBA blit invalidates it. No palette binding is
retained between frames, cameras or releases.

The DemoDisc N64 menu uses these per-platform settings:

| Asset | Format | Maximum resolution | Font raster size |
| --- | --- | --- | --- |
| Helengine logo | Indexed8/A8 | 256 | — |
| Main menu background | Indexed8/A8 | 64 | — |
| DemoDisc body atlas | Indexed8/A8 | 512 | 32 px |
| DemoDisc title atlas | Indexed8/A8 | 512 | Existing authored size |
| Default/FPS atlas | Indexed8/A8 | 512 | Existing generated size |

The source `.hasset` overrides belong to the DemoDisc project and select N64
only. Menu layout and glyph metrics remain independent of palette storage.
Validation artifacts and the active ROM use the fixed
`builds/helengine-n64/menu-20261007` directory.

The authored host uses the Core's measured update clock, backed by libdragon's
`get_ticks_us`, so menu animation advances by the actual elapsed frame time.
At 20 FPS, steady updates apply approximately 50 ms instead of a fixed 16.7 ms.
The first eight frame deltas are logged for startup diagnostics.
