# N64 generated runtime compatibility

This document records the generated Core checkpoint for the N64 platform.

## Task 1 status

The platform metadata and restricted material schema are implemented in the N64
builder project. The builder currently rejects `BuildAsync` with
`N64BUILD_NOT_IMPLEMENTED` until generated Core sources and the DragonFS native
packager are available. No generated C++ source is checked in or rewritten.

The current native host remains the standalone diagnostic cube. Its baseline
source revision is `415035a8d18b9320435203ddbd6292c5f3ab3356` on branch
`n64-v01`. A fresh baseline ROM hash must be recorded from the pinned Docker
image before the authored mode is enabled.

## Published contract

- Platform: `n64`
- Graphics profile: `n64-unlit`
- Material schema: `n64-unlit-v1`
- Build profiles: `debug`, `release`
- Serialization: generated profile declares `LittleEndian`; cooked payloads
  must be decoded through generated serializers on the target rather than
  copied into native structs.
- Runtime material resolution: cooked platform-owned payloads.
- Runtime storage: loose files staged into DragonFS and exposed as
  content-relative `rom:/` paths.
- Unsupported features: textures, lighting, transparency, audio, physics,
  input, UI, animation, and lights are rejected by metadata or cooking.

## Generated ABI record

No generated Core source has been emitted for this checkout yet. Once the
normal editor build graph generates C++, append the exact Core initialization,
`RenderManager3D`, content-stream, scene-manager, and model/material override
declarations here, together with the generator command, profile settings, and
file manifest. Do not infer declarations from this plan.
