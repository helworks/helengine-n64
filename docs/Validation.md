# N64 native host validation

## Authored build evidence

The corrected authored build was run from the N64 implementation checkout with the
pinned Docker image:

```powershell
pwsh -NoProfile -File scripts/build.ps1 -Clean -Mode authored `
  -GeneratedCoreRoot C:\dev\helworks\builds\helengine-n64\v02\generated\core-probe12 `
  -FilesystemRoot C:\dev\helworks\builds\helengine-n64\v02\filesystem-authored
```

It linked the generated Core unity source, the N64 runtime adapters, the
builder-owned startup catalog, and a DragonFS image. The corrected checkpoint
produced:

- ROM SHA-256: `f1a4122c912503edbe4e222f1303e59e7df76c65c14cd5a17697f5edbf7e39cd`
- ROM bytes: `802816`
- ELF load image from the linker size report: text `737240`, data `192257`,
  bss `9576`, total `939073`
- Toolchain image: `sha256:efceec9ed6d810b56388c508f4121597bfbba70d41a6dfeee4d8cad5f295fc32`
- Compiler: `mips64-elf-g++ (GCC) 16.2.0`
- Video: 320x240, 16-bit color, two buffers, 16-bit depth, `AA_NONE`

The staged fixture was generated through the editor `AssetSerializer` and
`EditorPlatformBuildScenePackager`, then placed under
`C:\dev\helworks\builds\helengine-n64\v02\filesystem-authored`:

- `cooked/scenes/authored.hasset`: 754 bytes
- `cooked/engine/models/cube.hasset`: 913 bytes
- `cooked/engine/materials/standard.hasset`: 80 bytes

The startup catalog names `cooked/scenes/authored.hasset`, and the DFS image
contains that exact path plus both dependencies. The earlier
`filesystem-probe` tree is retained as evidence only; its
`scenes/authored.scene` file was five literal ASCII bytes (`probe`) and is not
used by the corrected ROM. The build script emitted the ROM and ELF
successfully; its post-build `Get-FileHash` bookkeeping step is unavailable in
the `rtk powershell` wrapper, so the SHA-256 above was computed independently.

## Regression checks

- `dotnet test builder.tests/helengine.n64.builder.tests.csproj --no-restore
  -p:BuildProjectReferences=false`: 8 passed.
- C++ generator writer/harness focused tests: 12 passed.
- Shared `AliasedContentStreamSourceTests`: 4 passed.
- `scripts/test-host.ps1`: frame-clock and N64 content-source tests passed;
  the latter verifies both low-level DFS calls receive paths without `rom:/`.
- Clean diagnostic mode rebuild: 342,045-byte ELF and a distinct ROM hash,
  preserving the standalone multicolored cube path.

## Runtime limits

The ROM was not run in an emulator or on hardware in this environment. The
authored runtime logs are compiled into the image and report DragonFS/Core
readiness, startup scene ID, camera/drawable counts, model/material IDs, first
authored draw, and first completed frame when executed. Those target-side logs
and the editor-to-ROM visual proof remain unverified.
