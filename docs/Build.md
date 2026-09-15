# N64 native host build

This milestone builds a standalone native libdragon/Tiny3D ROM. It does not
use the Helengine editor, generated runtime, scene exporter, or platform
builder.

## Prerequisites

- Docker Engine 29.6.1 or a compatible Docker Engine.
- PowerShell 7 or Windows PowerShell.
- The pinned revisions and immutable base image in dependencies.json.

Run setup once from the repository root:

    ./scripts/setup-toolchain.ps1

The setup script clones or verifies the pinned dependency checkouts, builds the
toolchain image, records its immutable image digest and compiler version, and
refuses to overwrite locally modified dependency checkouts.

## Build

    ./scripts/build.ps1 -Clean -Debug
    ./scripts/build.ps1 -Clean

-Clean runs make clean and then performs a complete build. -Debug enables
libdragon RDPQ diagnostics. A successful build produces:

- helengine_n64.z64
- build/helengine_n64.elf
- build/helengine_n64.map
- ignored build/build-record.json with hashes, image identities, and video
  settings

The ROM uses 320x240, 16-bit color, two display buffers, one SDK-owned 16-bit
depth surface, GAMMA_NONE, FILTERS_RESAMPLE, and AA_NONE. The native host
owns the display/RDPQ/Tiny3D lifecycle and shows a rotating six-face vertex
colored cube.

## Host tests

The frame clock has a native host test runner:

    ./scripts/test-host.ps1

The tests cover normal elapsed time, the 100 ms delta cap, backward timestamps,
and bounded angle wrapping.

## Emulator

Set the explicit ares path and launch a freshly built ROM:

    $env:HELENGINE_N64_ARES_PATH = 'C:\path\to\ares.exe'
    ./scripts/launch_in_emulator.ps1 -ArtifactPath .\helengine_n64.z64

The launcher validates both paths and does not launch an old ROM after a build
failure. Runtime rendering evidence is recorded separately in Validation.md.
