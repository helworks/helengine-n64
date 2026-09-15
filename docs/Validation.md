# N64 native host validation

## Authored build evidence

The clean authored build was run from the N64 implementation checkout with the
pinned Docker image:

```powershell
pwsh -NoProfile -File scripts/build.ps1 -Clean -Mode authored `
  -GeneratedCoreRoot C:\dev\helworks\builds\helengine-n64\v02\generated\core-probe12 `
  -FilesystemRoot C:\dev\helworks\builds\helengine-n64\v02\filesystem-probe
```

It linked the generated Core unity source, the N64 runtime adapters, the
builder-owned startup catalog, and a DragonFS image. The final checkpoint
recorded:

- ROM SHA-256: `83c91e026932a11be736d7bcc432d34bebdf29f503b383210c5632d960a58e16`
- ROM bytes: `786432`
- ELF load image from the linker size report: text `736728`, data `192165`,
  bss `9576`, total `938469`
- Toolchain image: `sha256:efceec9ed6d810b56388c508f4121597bfbba70d41a6dfeee4d8cad5f295fc32`
- Compiler: `mips64-elf-g++ (GCC) 16.2.0`
- Video: 320x240, 16-bit color, two buffers, 16-bit depth, `AA_NONE`

The staged fixture was under
`C:\dev\helworks\builds\helengine-n64\v02\filesystem-probe` and was passed
to `mkdfs` through `HELENGINE_N64_FILESYSTEM_ROOT`. The managed builder removes
and recreates its staging root before each build, so deleted or renamed assets
cannot remain in an incremental DFS image.

## Regression checks

- `dotnet test builder.tests/helengine.n64.builder.tests.csproj --no-restore
  -p:BuildProjectReferences=false`: 8 passed.
- C++ generator writer/harness focused tests: 12 passed.
- Shared `AliasedContentStreamSourceTests`: 4 passed.
- Clean diagnostic mode rebuild: 342,045-byte ELF and a distinct ROM hash,
  preserving the standalone multicolored cube path.

## Runtime limits

The ROM was not run in an emulator or on hardware in this environment. The
authored runtime logs are compiled into the image and report DragonFS/Core
readiness, startup scene ID, camera/drawable counts, model/material IDs, first
authored draw, and first completed frame when executed. Those target-side logs
and the editor-to-ROM visual proof remain unverified.
