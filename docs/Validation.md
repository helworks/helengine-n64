# N64 native host validation

## Build and test evidence

These checks pass from the repository root on the pinned Docker toolchain:

    ./scripts/test-host.ps1
    frame-clock-tests: PASS

    ./scripts/build.ps1 -Clean -Debug
    ROM SHA-256: 3ac7f383935285d1fca6d9564bb4045da07a6642f4490a3fbf9f4faa5ca0f851
    ROM bytes: 278528
    ELF bytes: 1781640
    MAP bytes: 1460111

The clean debug build compiled main.cpp, the native video host, cube mesh,
Tiny3D renderer, and frame clock, then linked and converted the ROM
successfully. build/build-record.json records:

- toolchain image sha256:efceec8f134899c8f199c91c9eee2d26ae3bb562fc5d9702b9f8b12734981a7b
- build image sha256:34b5a306bcca70524d0edebb7a8d4d197249dd0ccd35959b969e8a5962918bd6
- GCC mips64-elf-g++ (GCC) 16.2.0
- 320x240, 16-bit color, two buffers, one 16-bit depth surface, AA_NONE

The source diff and staged diff were checked with git diff --check.

## Code-level checkpoint evidence

- Nintendo64Video calls display_init(RESOLUTION_320x240, DEPTH_16_BPP,
  2, GAMMA_NONE, FILTERS_RESAMPLE), obtains display_get_zbuf(), attaches
  RDPQ, and ends each frame with rdpq_detach_wait() followed by
  display_show().
- Nintendo64Renderer sets RDPQ_COMBINER_SHADE, AA_NONE, depth compare and
  update, disabled blender/fog, zero lights, and
  T3D_FLAG_NO_LIGHT | T3D_FLAG_DEPTH | T3D_FLAG_CULL_BACK.
- Nintendo64CubeMesh keeps its 24 logical vertices and 36 indices persistent
  across frames and validates each face winding against its outward normal.
- FrameClock uses get_ticks_us(), caps elapsed time at 100 ms, ignores a
  backward timestamp, and drives 0.35/0.55 radians-per-second rotation.

## Runtime evidence

Runtime rendering was not validated in this environment. No ares.exe is on
PATH, HELENGINE_N64_ARES_PATH is unset, and the explicit repository emulator
locations contain no ares executable. Running the launcher without a configured
emulator fails before process launch with:

    Set -EmulatorPath or HELENGINE_N64_ARES_PATH to an ares executable.

No hardware capture is available. The successful ROM build is therefore build
evidence only; it is not a claim that the cube was visually rendered.
