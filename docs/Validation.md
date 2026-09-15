# N64 native host validation

## Task 1 checkpoint

The target configuration is:

- 320x240 framebuffer, 16-bit color, two display buffers.
- One SDK-owned 16-bit depth surface from `display_get_zbuf()`.
- `GAMMA_NONE` and `FILTERS_RESAMPLE`.
- Explicit startup and first-frame diagnostics: `n64:video-ready` and
  `n64:first-frame-complete`.
- End-of-frame ownership boundary: `rdpq_detach_wait()` followed by
  `display_show(CurrentFrame)`.

The build and runtime results are intentionally left as observed evidence and
must be updated after the pinned Docker build and ares run. A compiler result
alone is not a rendering result.
