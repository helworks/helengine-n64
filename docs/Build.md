# N64 native host build

This milestone builds a standalone native libdragon/Tiny3D ROM. It does not
use the Helengine editor, generated runtime, scene exporter, or platform
builder.

## Prerequisites

- Docker Engine 29.6.1 or a compatible Docker Engine.
- PowerShell 7 or Windows PowerShell.
- The pinned revisions and immutable base image in `dependencies.json`.

Run the setup once from the repository root:

```powershell
./scripts/setup-toolchain.ps1
```

The script refuses to overwrite locally modified dependency checkouts and
records the resulting toolchain image/compiler identity in `dependencies.json`.

## Build

```powershell
./scripts/build.ps1 -Clean -Debug
./scripts/build.ps1 -Clean
```

The successful build produces `helengine_n64.z64`, `build/helengine_n64.elf`,
`build/helengine_n64.map`, and the ignored `build/build-record.json`. The ROM
uses 320x240, 16-bit color, two display buffers, a 16-bit SDK-owned depth
surface, `GAMMA_NONE`, `FILTERS_RESAMPLE`, and Tiny3D/RDPQ antialiasing disabled
for the cube scene.

## Emulator

Set the explicit ares path and launch a freshly built ROM:

```powershell
$env:HELENGINE_N64_ARES_PATH = 'C:\path\to\ares.exe'
./scripts/launch_in_emulator.ps1 -ArtifactPath .\helengine_n64.z64
```

The launcher validates both paths and does not launch an old ROM after a build
failure. Runtime rendering evidence is recorded separately in `Validation.md`.
