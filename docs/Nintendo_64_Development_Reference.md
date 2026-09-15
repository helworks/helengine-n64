# Nintendo 64 Development Reference

A practical engineering manual for original Nintendo 64 software built with freely available tools. It covers the CPU, RSP and RDP, memory and DMA, C and C++, project and asset paths, rendering architecture, audio, input, storage, debugging, and release preparation.

**Recommended baseline:** libdragon, its MIPS GCC toolchain, Tiny3D for native 3D, RDPQ for materials and 2D, and ares for everyday debugging. Use a development cartridge for regular tests on a real console. The proposed engine design starts at 320 × 240 with 16-bit color and a 4 MiB memory budget; these are engineering choices, not platform limits.[^1]

“Free” here means no proprietary Nintendo SDK license or SDK royalty is required for the selected development route. Open-source and bundled asset licenses still apply. The license review below identifies concrete obligations and a boot-font exception; it does not certify compliance with every country’s copyright, trademark, hardware, or anticircumvention law.

Hardware behavior is distinguished from SDK behavior and proposed engine policy throughout. Version-sensitive paths and interfaces are anchored to the source snapshots listed in section 2. Code and commands are source-checked reference examples, not a claim of a completed build or hardware test.

## Contents

[1 Toolchain and licensing](#1-toolchain-and-licensing)  
[2 Reproducible setup and version control](#2-reproducible-setup-and-version-control)  
[3 Project structure and a first ROM](#3-project-structure-and-a-first-rom)  
[4 The machine and its memory map](#4-the-machine-and-its-memory-map)  
[5 CPU programming with C and C++](#5-cpu-programming-with-c-and-c)  
[6 Shared memory and DMA ownership](#6-shared-memory-and-dma-ownership)  
[7 RSP microcode and scheduling](#7-rsp-microcode-and-scheduling)  
[8 RDP rendering and video output](#8-rdp-rendering-and-video-output)  
[9 Textures and asset budgets](#9-textures-and-asset-budgets)  
[10 Building the graphics engine](#10-building-the-graphics-engine)  
[11 Filesystems and binary formats](#11-filesystems-and-binary-formats)  
[12 Audio input and persistent saves](#12-audio-input-and-persistent-saves)  
[13 Timing profiling and optimization](#13-timing-profiling-and-optimization)  
[14 Debugging and hardware deployment](#14-debugging-and-hardware-deployment)  
[15 Implementation plan and release criteria](#15-implementation-plan-and-release-criteria)  
[16 Source path navigator](#16-source-path-navigator)  
[Sources](#sources)

## 1 Toolchain and licensing

### Recommended components

| **Component**    | **Role**                          | **License and practical condition**                                                                    |
|------------------|-----------------------------------|--------------------------------------------------------------------------------------------------------|
| libdragon        | Runtime SDK and RDPQ or RSPQ      | Core uses the Unlicense. Inspect individual files and bundled dependencies.                            |
| Tiny3D           | RSP 3D pipeline and model runtime | MIT. Retain its copyright and permission notice when distributing covered code or binaries.            |
| GCC and binutils | C and C++ compilation and linking | GPL tool distributions. Covered GCC runtime code has the Runtime Library Exception.                    |
| Newlib           | Embedded C library                | Collection of licenses, largely permissive. Preserve applicable notices.                               |
| Fast64           | Blender material authoring        | GPL-3.0 tool. Use original assets and the Tiny3D glTF route.                                           |
| ares             | Host emulator and debugger        | Open source with a component license collection. It runs on the host.                                  |
| SummerCart64     | Hardware upload and debug option  | Open hardware and software project; repository GPL-3.0. Cartridge cost is separate from SDK licensing. |

The SDK and renderer have affirmative grants permitting use, modification and distribution. Tiny3D also states that its microcode was independently written rather than based on proprietary code. These are the reasons to choose this route for new software.[^2]

Keep two inventories: code or assets that enter the ROM, and tools used only on the host. A GPL compiler or converter does not automatically impose its license on original output; generated output that incorporates protected tool code or templates requires its own analysis. GCC’s runtime exception grants additional permission for qualifying compilation and combinations with independent modules. Redistributing a compiler, converter or modified SDK is a separate activity with its own obligations.[^3]

### Boot code and the bundled font exception

Use libdragon’s own IPL3. It is built into n64tool and selected by the normal n64.mk build process, so a new project does not need to extract the boot block from a commercial ROM. The open bootloader initializes RDRAM and loads an appended ELF. Do not substitute an old tutorial’s copyrighted boot binary or Nintendo microcode.[^4]

The boot error screen uses the “Simple 5x8” font, credited separately in boot/assets/README.md. Its FFC license allows commercial use and application embedding, but restricts modifying the typeface and distributing it separately. Thus the entire source tree should not be described as uniformly public domain or uniformly libre. For a strict all-libre distribution, replace the font with a wholly original glyph sheet or an appropriately licensed font, regenerate the boot assets, and rebuild the bootloader and host ROM tool; merely changing the game’s UI font is insufficient.[^5]

A practical compliance record should contain each shipped component’s exact revision, its license text, any required attribution, and the origin of every font, sound, texture and model. Audit optional codecs and converter dependencies when enabling them. Example assets are useful for experiments, but their presence in an open repository is not proof that every asset may be redistributed in a commercial game.

### Materials to exclude from this workflow

Keep leaked Nintendo SDKs, libultra binaries or source, proprietary RSP microcode, retail boot-code extracts, and commercial game assets out of the project. Do not use decompilation projects as a convenient source of Nintendo-owned content. Hardware-register facts and independently implemented APIs can guide engineering without importing those materials. Use original branding rather than implying Nintendo approval.

A lawful tool license is necessary evidence, not an all-purpose legal clearance. A physical cartridge release, proprietary firmware distribution or product advertised in a particular country warrants a review focused on that product and jurisdiction. This document makes no blanket claim that flashcart sale or circumvention is lawful everywhere.

### Alternative development routes

For a 2D title, libdragon trunk plus RDPQ avoids the additional dependency on Tiny3D. For a port of an older OpenGL renderer, libdragon’s OpenGL implementation offers familiar fixed-function interfaces, but it has platform-specific performance guidance and incomplete features. Prefer recorded display lists and check for CPU fallback. It is not a modern shader API.[^6]

For an editor-first workflow, Pyrite64 supplies an MIT-licensed engine and editor built on libdragon and Tiny3D. It offers scenes, rendering, collision, audio and glTF import, but its maintainers still describe it as early development with breaking changes. It is a credible alternative to writing all the engine infrastructure; audit its vendored dependencies as part of adoption.[^7]

## 2 Reproducible setup and version control

The source snapshots for this reference are libdragon preview commit 39d0d6096130836a65710da6730290701d060272 dated 15 September 2026 and Tiny3D main commit 73d822f3af7c85cc1b93eaa4546504ba0a4ea156 dated 30 August 2026. They are independently inspected snapshots, not a certified compatibility pair. Establish a tested pair by building Tiny3D’s examples, then freeze both revisions in the project.

Tiny3D’s documented dependency is libdragon preview. The libdragon documentation distinguishes stable trunk from preview, where interfaces can change. Record the compiler package or container digest as well as the SDK commits; a branch name or a moving “latest” download is insufficient for reproducibility.[^8]

### Native Linux or WSL setup

Use the official libdragon installation page to obtain the current prebuilt toolchain matching the host architecture. The Debian or Ubuntu package route installs under /opt/libdragon. Install the normal host build prerequisites, then build libdragon itself. The compiler package and runtime SDK are separate pieces. Windows also has an MSYS2 route, and the documented container route supports Linux, Windows and macOS.[^9]

``` bash
# After installing the official host-specific toolchain package:
export N64_INST=/opt/libdragon
export PATH="$N64_INST/bin:$PATH"
mkdir -p "$HOME/dev/n64"
cd "$HOME/dev/n64"
git clone --branch preview https://github.com/DragonMinded/libdragon.git
cd libdragon
git checkout 39d0d6096130836a65710da6730290701d060272
./build.sh
cd ..
git clone https://github.com/HailToDodongo/tiny3d.git
cd tiny3d
git checkout 73d822f3af7c85cc1b93eaa4546504ba0a4ea156
./build.sh
```

These commands intentionally leave the host-specific toolchain download and package installation to the official instructions. Avoid spaces in the SDK installation path. If building the compiler from source, libdragon provides tools/build-toolchain.sh; this is a substantial prerequisite build, not something every project should repeat.

The inspected toolchain script identifies GCC 16.2.0, binutils 2.45 and Newlib 4.4.0.20231231. It enables C and C++. The inspected n64.mk uses the VR4300 target, o64 ABI, GNU C17 and GNU C++17 defaults, and links normal programs with g++. Respect its target and linker flags; generic desktop or Linux MIPS flags are not a substitute.[^10]

### Path map for the installed SDK

| **Path**                       | **Meaning**                                                        |
|--------------------------------|--------------------------------------------------------------------|
| \$N64_INST/bin/mips64-elf-gcc  | N64 C compiler, not the host gcc.                                  |
| \$N64_INST/bin/mips64-elf-g++  | N64 C++ compiler and normal final link driver.                     |
| \$N64_INST/include/n64.mk      | Build rules imported by your Makefile.                             |
| \$N64_INST/mips64-elf/include/ | Target headers, including libdragon.h and subsystem headers.       |
| \$N64_INST/mips64-elf/lib/     | Target libraries and linker scripts, including n64.ld.             |
| \$N64_INST/bin/                | Host asset tools such as mksprite, mkdfs, mkasset and audioconv64. |
| \$N64_INST/include/t3d.mk      | Tiny3D integration after system installation.                      |
| \$N64_INST/bin/gltf_to_t3d     | Tiny3D model converter after installation.                         |
| third_party/tiny3d/t3d.mk      | Alternative project-local Tiny3D integration.                      |

The apparent split between include/n64.mk and mips64-elf/include is intentional: one contains host build rules, the other target headers. Tiny3D’s local t3d.mk points at its local library and converter; its installed integration file uses the installed versions.[^11]

### Evidence to retain with each build

``` bash
mips64-elf-g++ --version
git -C third_party/libdragon rev-parse HEAD
git -C third_party/tiny3d rev-parse HEAD
make D=1
mips64-elf-size build/game.elf
mips64-elf-nm --print-size --size-sort build/game.elf
sha256sum game.z64
```

Adapt the repository paths to the actual checkout. Archive the unstripped ELF, map file, final ROM hash and asset-conversion configuration together. A crash address is only useful when matched to the exact executable. Rebuild all target libraries after an ABI or toolchain change.

## 3 Project structure and a first ROM

The following is a proposed project layout. It keeps authored assets separate from converted ROM assets and makes the N64 backend small enough to replace or test independently.

| **Project path**  | **Responsibility**                                                           |
|-------------------|------------------------------------------------------------------------------|
| src/main.cpp      | Startup and the top-level loop.                                              |
| src/game/         | Gameplay, world simulation and scene transitions.                            |
| src/engine/       | Portable math, asset IDs, collision and scene interfaces.                    |
| src/platform/n64/ | Renderer, audio, input, timing, save and debug adapters.                     |
| assets_src/       | Original models, textures, sounds and fonts; never silently overwrite these. |
| filesystem/       | Generated files packed into DragonFS.                                        |
| tools/            | Project-owned asset cooker and validation scripts.                           |
| third_party/      | Pinned external repositories or recorded dependency locations.               |
| licenses/         | Third-party notices and asset provenance records.                            |
| build/            | Objects, dependency files, ELF, map and DFS image.                           |
| game.z64          | Final ROM image emitted by the build.                                        |

### Minimal C++ framebuffer example

This example deliberately has no external assets or Tiny3D dependency. Save it as src/main.cpp and use the Makefile below. It initializes the display, draws a rectangle using the RDP, and hands presentation to the asynchronous attachment API. The declarations were checked against the pinned libdragon source; compile and run it as the first local acceptance test.[^12]

``` cpp
#include <libdragon.h>

int main() {
    debug_init_isviewer();
    debug_init_usblog();
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2,
                 GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    joypad_init();
    for (;;) {
        joypad_poll();
        surface_t* frame = display_get();
        rdpq_attach(frame, nullptr);
        rdpq_clear(RGBA32(16, 18, 24, 255));
        rdpq_set_mode_fill(RGBA32(160, 120, 220, 255));
        rdpq_fill_rectangle(112, 84, 208, 156);
        rdpq_detach_show();
    }
}
```

### Minimal Makefile

``` makefile
.DEFAULT_GOAL := all
BUILD_DIR := build
SOURCE_DIR := src
N64_ROM_TITLE := "N64 First ROM"
N64_ROM_SAVETYPE := none
N64_ROM_EXPANSIONPAK := unused
include $(N64_INST)/include/n64.mk

all: game.z64
$(BUILD_DIR)/game.elf: $(BUILD_DIR)/main.o

-include $(wildcard $(BUILD_DIR)/*.d)
.PHONY: all
```

Run make from the project root. The inherited rules build build/game.elf, build/game.map and game.z64. This minimal Makefile avoids custom recipe lines; when adding recipes, GNU Make normally requires a literal tab. Add each additional source object to the ELF prerequisites. Do not let a pattern for only root-level files silently omit nested engine sources.[^13]

Success means the rectangle is visible in ares and on hardware, input polling does not hang, and the same ROM starts with and without an Expansion Pak. Add one dependency at a time after this baseline: DragonFS, a sprite, audio, then Tiny3D. A successful desktop compile of portable code does not validate the ROM build.

## 4 The machine and its memory map

| **Unit**   | **Hardware fact**                                                              | **Engineering consequence**                                                               |
|------------|--------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------|
| VR4300 CPU | 93.75 MHz; MIPS III class, 64-bit integer registers, COP0 and FPU.             | Run game logic, visibility, resource management and command submission here.              |
| CPU caches | 16 KiB instruction cache, 32-byte lines; 8 KiB data cache, 16-byte lines.      | Code and data locality matter. DMA is not automatically coherent with CPU cache.          |
| RSP        | 62.5 MHz programmable scalar and vector processor; 4 KiB IMEM plus 4 KiB DMEM. | Run specialized graphics and audio microcode; working sets are tiny.                      |
| RDP        | 62.5 MHz fixed-function rasterizer; 4 KiB texture memory.                      | Rasterization and blending are hardware functions; arbitrary fragment shaders are absent. |
| RDRAM      | 4 MiB standard; 8 MiB with Expansion Pak.                                      | Code, meshes, textures, sound buffers and render targets compete for one pool.            |
| VI and AI  | Video scanout and audio output engines.                                        | Rendering completion, presentation timing and audio deadlines are distinct events.        |

Clock and cache values come from libdragon’s hardware support. RSP memory and vector details are corroborated by independent emulator implementation notes and ares. “64-bit console” does not mean every subsystem, pointer or texture operation is 64-bit.[^14]

The system has no separate general-purpose VRAM pool. The RDP has small internal memories, but textures and framebuffers live in shared RDRAM. RDRAM also carries hidden bits used for graphics coverage and depth bookkeeping; they are not additional ordinary CPU heap capacity. The Expansion Pak increases memory capacity, not CPU or RCP clock speed.

### Physical addresses and CPU aliases

| **Physical address**    | **Region**                                            |
|-------------------------|-------------------------------------------------------|
| 0x00000000–0x003FFFFF   | Base 4 MiB RDRAM.                                     |
| 0x00400000–0x007FFFFF   | Expansion Pak RDRAM when present.                     |
| 0x03F00000              | RDRAM register window.                                |
| 0x04000000 / 0x04001000 | RSP DMEM / IMEM, each 4 KiB.                          |
| 0x04040000 / 0x04080000 | RSP control and DMA / program-counter register block. |
| 0x04100000 / 0x04200000 | RDP command / span-register windows.                  |
| 0x04300000 / 0x04400000 | MI interrupt control / VI video registers.            |
| 0x04500000 / 0x04600000 | AI audio / PI cartridge DMA registers.                |
| 0x04700000 / 0x04800000 | RI RDRAM interface / SI PIF transfer registers.       |
| 0x08000000              | Cartridge save-memory domain, device dependent.       |
| 0x10000000              | Normal cartridge ROM mapping begins here.             |
| 0x1FC00000 / 0x1FC007C0 | PIF boot ROM / 64-byte PIF RAM.                       |

These are hardware windows, not file offsets or allocations. KSEG0 addresses 0x80000000–0x9FFFFFFF are cached direct mappings; KSEG1 addresses 0xA0000000–0xBFFFFFFF are uncached direct mappings. For example, CPU pointers 0x80001000 and 0xA0001000 refer to physical 0x00001000. TLB-mapped regions require actual address translation; masking arbitrary pointers is unsafe.[^15]

Use PhysicalAddr, VirtualCachedAddr and VirtualUncachedAddr only according to their documented address domains. Hardware consumers generally use physical addresses, whereas C and C++ use CPU virtual pointers. A RAM address, cartridge address, DragonFS offset and Tiny3D segment reference may all be integers but are not interchangeable.[^16]

### Boot and executable layout

The console’s boot hardware starts the cartridge boot sequence; libdragon supplies the cartridge-side IPL3 implementation. The normal build embeds an ELF payload rather than assuming a retail game’s fixed binary layout. The inspected linker script places interrupt vectors at 0x80000000 and the main text region at 0x80000400. Treat these as properties of this linker script, not mandatory addresses for every N64 program.[^17]

Do not assume that a ROM byte offset is the same as a loaded RAM address. Startup loads segments, zeroes BSS, initializes runtime state and eventually calls main. Keep the linker’s reserved regions intact. An unusually large static binary can fail before main even when the ROM fits on a cartridge; the bootloader must also fit its loadable segments into available RAM.

Use .z64 as the canonical ROM output of the supplied build. The .v64 rule in n64.mk performs a byte-pair swap; changing the filename alone does not convert byte order. Keep the original .z64 and its hash as the release artifact and let the intended loader handle supported formats.[^18]

## 5 CPU programming with C and C++

C++ is a practical choice for N64 software. The selected toolchain builds both C and C++, and n64.mk already has .cpp compilation rules and uses g++ for the normal executable link. Classes, templates, constexpr, namespaces and RAII do not inherently require an operating system. Their suitability depends on generated code, allocation behavior and runtime dependencies.[^19]

Start with ordinary optimized C++ and measure. Use small value types, explicit ownership and contiguous containers. Reserve variable-capacity buffers during loading, then avoid allocation in the frame loop. Templates can remove dispatch overhead but can also expand code beyond the instruction cache. Virtual calls are possible; their cost depends on frequency, locality and branch behavior rather than a universal prohibition.

The inspected target uses o64 with 32-bit application pointers and 64-bit-capable integer registers. Do not confuse it with the n64 ABI, where the name describes a different ABI. Use fixed-width integer types for binary formats, and verify assumptions with static_assert. Never dump a native C++ object directly into a portable file: padding, pointers, vtables, endianness and compiler ABI all make that fragile.[^20]

``` cpp
#include <cstdint>
static_assert(sizeof(void*) == 4);
static_assert(sizeof(std::uint32_t) == 4);
static_assert(sizeof(std::uint64_t) == 8);
```

### Runtime choices

Use float for typical world and camera calculations unless measurements justify fixed point. The CPU has a floating-point unit; the RSP does not. Avoid accidental double promotion in frequently executed math, and benchmark division, square root and transcendental functions before choosing approximations. Convert to the renderer’s fixed-point representation at an explicit boundary.

Exceptions, RTTI, iostreams and general-purpose containers are not all-or-nothing language requirements. A small engine may disable exceptions and RTTI to reduce complexity or size, but this is a project policy, not an N64 hardware rule. Apply such flags consistently across relevant code and dependencies. RAII still works without exceptions for normal scope exit. Prefer explicit initialization over global objects that touch video, audio or files before main.

The SDK’s default floating-point flags are performance-oriented. If deterministic simulation or precise IEEE behavior matters, inspect and override the flags deliberately rather than assuming desktop equivalence. Measure the actual executable and map file: a tiny source change can pull in formatting, allocation or math support code.[^21]

### Low level CPU details that matter

The CPU exposes COP0 for exceptions, timing and memory control, and COP1 for floating-point work. It has branch delay slots, so handwritten assembly and debugger stepping do not always follow the intuitive next-line model. Naturally aligned halfword, word and doubleword accesses are required by the ordinary load and store instructions; use serialization or appropriate unaligned-access helpers for packed data.

Keep CPU assembly isolated behind narrow interfaces. Let the SDK toolchain handle processor errata and target-specific code generation. When an optimization changes scheduling or inserts handwritten multiply sequences, inspect the VR4300 handling in the pinned toolchain rather than copying generic MIPS code. The reviewed source includes explicit VR4300 workaround logic.[^22]

Use interrupt callbacks for short bookkeeping and signaling. Avoid blocking, lengthy filesystem work or heap-heavy operations inside them. A game can run effectively with a single main loop plus hardware queues. Preview threading is an SDK feature with thread-safety limitations; std::thread-style desktop assumptions do not follow from having a C++ compiler.[^23]

## 6 Shared memory and DMA ownership

The CPU’s data cache is not automatically kept coherent with RSP, RDP, PI, AI or VI accesses to RDRAM. A CPU store can be newer than RAM; conversely, a device write can be newer than a cached CPU copy. Cache maintenance and completion ordering solve different problems: writing back a buffer does not wait for a device, and waiting does not repair a stale cache line.[^24]

| **Transfer direction**                  | **Required ownership pattern**                                                                                                               |
|-----------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------|
| CPU prepares device input               | Finish writing, make the data visible in RAM, then submit. Keep it alive and unchanged until the consumer finishes.                          |
| Device writes CPU output                | Prepare destination cache lines before DMA; grant exclusive ownership to the device. Wait for completion, then ensure CPU reads observe RAM. |
| CPU uses an uncached allocation         | Use the uncached address consistently. It avoids dirty CPU cache copies but does not eliminate DMA completion or lifetime requirements.      |
| Device produces data for another device | Sequence the producer and consumer correctly, including RDP synchronization where needed. CPU cache operations alone do not order devices.   |

For custom cached destinations, prevent dirty cache lines from being evicted over incoming device data. A robust ownership protocol cleans or invalidates the complete destination lines before the device writes, prevents CPU access while it is in flight, and invalidates again before CPU reads if needed by the API and access pattern. Never write back an old dirty CPU copy after a device has already produced the new result.

The data cache line is 16 bytes. Isolate DMA allocations on full cache-line boundaries so invalidation cannot discard an unrelated neighboring object. libdragon exposes data_cache_hit_writeback, data_cache_hit_invalidate, data_cache_hit_writeback_invalidate and uncached allocation helpers. Use free_uncached for memory created by malloc_uncached. SDK wrappers may already perform maintenance; read their contract before duplicating it.[^25]

### Cartridge I O

PI transfers move blocks between cartridge space and RDRAM. Access cartridge data through DragonFS, asset APIs, dma_read or io_read rather than ordinary memcpy from an assumed ROM pointer. Even a raw cartridge read can conflict with another PI transfer. In the reviewed low-level DMA API, raw transfers require an 8-byte-aligned RAM address, 2-byte-aligned peripheral address and even transfer length; the higher-level API handles more cases.[^26]

Larger sequential reads amortize transfer setup. Batch small asset records, prefetch scene chunks and avoid opening hundreds of tiny files in the hot loop. Align asset containers for the actual loader and DMA path. Treat the best transfer size as a benchmark result for the target cartridge, not a universal platform constant.

### Frame resources and completion

Use a ring of per-frame render resources. Each slot owns its matrices, generated vertices, transient command data and completion token. Reuse a slot only after its last consumer is finished. CPU scope is not GPU lifetime: a matrix on the stack may be gone before the RSP reads it, and a mesh may still be referenced by a recorded command block after its C++ owner has been destroyed.

An RSPQ syncpoint proves the RSP reached a particular position; do not infer that all RDP pixels have also finished. Use the appropriate RDP full-completion boundary for render-target reuse or readback. In the reviewed API, rspq_wait waits for queued RSP work and its RDP drawing; rdpq_detach_wait provides a rendering-completion path. rdpq_detach_show schedules display after rendering without forcing the main CPU to wait immediately.[^27]

A conservative first implementation can wait at frame end to establish correctness. Replace that global stall with explicit resource lifetimes and granular completion only after correctness is stable. Double or triple buffering by itself is not a proof that a particular matrix or texture is safe to overwrite.

## 7 RSP microcode and scheduling

The RSP has 32 scalar 32-bit registers and 32 vector registers, each holding eight 16-bit lanes. Its vector accumulator supports wider intermediate results across lanes. It has no ordinary CPU floating-point unit and no general-purpose operating-system environment. Program code occupies 4 KiB IMEM; scalar and vector data use 4 KiB DMEM. RDRAM transfers must be staged through its DMA facilities.[^28]

RSP code is called microcode, but it is programmable software. Tiny3D performs geometric work there; libdragon’s mixer also uses it. Those workloads share one RSP and do not run simultaneously on separate vector engines. Their execution can overlap CPU logic and RDP rasterization. Heavy animation, lighting or custom effects can therefore steal time from audio processing.

RSPQ supplies a shared command queue and overlay mechanism. An overlay is loaded when its command stream needs it; different libraries can coexist without the application manually replacing the entire RSP program. Queue blocks record reusable commands and reduce CPU submission work. For custom acceleration, implement an RSPQ-compatible overlay rather than taking exclusive control of the RSP while Tiny3D and audio are active.[^29]

### When custom microcode is worthwhile

Good candidates are regular vector operations on compact batches with enough arithmetic to amortize DMA: transform variants, skinning experiments, audio effects or a carefully measured data transform. Poor candidates are branch-heavy game logic, small one-off operations and code that constantly shuttles tiny buffers through DMEM. Write a scalar reference and compare output across boundary values before benchmarking speed.

libdragon recognizes an assembly file whose name begins with rsp and ends in .S as RSP microcode under n64.mk. The lower-level rsp.h APIs also support loading and running standalone programs, useful for focused experiments. These programs use a different instruction subset and assembly configuration from CPU code.[^30]

Tiny3D includes generated assembly, so ordinary users do not need a microcode compiler. Its source can be edited with RSPL, an Apache-2.0 project that produces RSP assembly and disclaims ownership of users’ generated output. The RSPL repository is transitioning from JavaScript to C++; pin the compiler used to regenerate microcode and follow that revision’s build instructions.[^31]

## 8 RDP rendering and video output

The RDP is the fixed-function part of the graphics pipeline. It consumes drawing commands, rasterizes screen-space triangles and rectangles, samples textures, combines colors, performs depth and coverage operations, blends with the framebuffer, and writes RDRAM. Transforming a world-space vertex into a rasterizable primitive is work for the CPU or RSP pipeline, not an automatic operation performed by the RDP.[^32]

Think of a frame as several overlapping jobs: the CPU decides what is visible and submits commands; the RSP transforms geometry and emits rasterizer commands; the RDP produces pixels; VI scans out a completed framebuffer. The next CPU frame may begin before the previous image is finished. This overlap is useful only when resource ownership is correct.

Perspective-correct texturing uses the projected vertex attributes supplied by the geometry pipeline. Clipping and triangle setup must be correct before rasterization, especially around the near plane. Use Tiny3D’s normal clipping path first; rejecting intersecting triangles or performing an unsafe perspective divide can create disappearing geometry and large screen artifacts.[^33]

### Cycle modes and material state

| **Mode or stage** | **Use and limitation**                                                                                   |
|-------------------|----------------------------------------------------------------------------------------------------------|
| Fill mode         | Fast rectangular clears and fills. It is not the normal textured-triangle pipeline.                      |
| Copy mode         | Special texture-copy path with restrictions; useful for suitable blits.                                  |
| One cycle         | Common textured or shaded drawing with a relatively simple combiner.                                     |
| Two cycle         | More complex combining, filtering or effects; adds raster work.                                          |
| Color combiner    | Limited fixed equations combine texture, vertex shade, primitive color, environment color and constants. |
| Blender and depth | Controls framebuffer blending, coverage and depth behavior; configure with the material.                 |

A useful combiner mental model is (A − B) × C + D with a restricted set of sources, potentially applied through two cycles. It is not a programmable fragment shader. RDPQ’s high-level mode API selects and adapts cycle settings for many combinations. Prefer it initially, then use raw modes only when a measured feature requires them.[^34]

There is no native modern PBR pipeline, compute shader, stencil-buffer API or arbitrary per-pixel program. Stylized lighting, baked light, vertex colors, environment mapping and multipass effects fit better. A sophisticated effect shown in a demo is evidence of a particular implementation, not proof that it is free at game scale.

### Depth and transparency

Use a 16-bit depth surface for the usual Z-buffer path. Clear through the SDK’s depth helper because the hardware representation is specialized. Draw opaque surfaces with depth testing and appropriate writes, then handle cutouts and translucent surfaces as separate material classes. Sort translucent content where needed and avoid assuming that alpha blending solves visibility ordering.[^35]

Coverage-based antialiasing is not equivalent to a desktop multisample buffer. Alpha, coverage and framebuffer blending interact with render modes. Validate foliage, particle edges, decals and overlapping transparent surfaces in ares and on a real display. Read-modify-write blending and repeated full-screen passes can saturate memory traffic before triangle throughput becomes the limit.

### VI and regional output

VI presents the framebuffer and applies configured output processing. Render resolution, display aspect ratio, filtering, interlace mode and TV timing are related but separate decisions. Begin with the SDK’s standard 320 × 240 configuration rather than programming VI registers directly. Keep important UI away from overscan edges and test legibility on the intended display chain.[^36]

Support PAL, NTSC and MPAL through the SDK’s TV-type handling. Approximately 50 Hz and 60 Hz presentation schedules should not change simulation speed. Use elapsed time or a fixed simulation step with presentation decoupled. Region-free boot metadata does not prove that a display mode will work identically on every television or video adapter.[^37]

## 9 Textures and asset budgets

TMEM is a 4 KiB texture working memory inside the RDP, not a limit on all textures stored in a game. Textures are loaded into it as drawing proceeds. Eight tile descriptors describe texture views and sampling state; they do not provide eight independent 4 KiB allocations. Mip levels and multiple textures share the same working memory.[^38]

| **Format**  | **Typical use**                                     | **Example payload**                                     |
|-------------|-----------------------------------------------------|---------------------------------------------------------|
| RGBA16      | Opaque or cutout color, 5/5/5 color and 1-bit alpha | 32 × 32 = 2,048 bytes; 64 × 32 = 4,096 bytes.           |
| RGBA32      | 8-bit channels, greater color or alpha precision    | 32 × 32 = 4,096 bytes, arranged across TMEM banks.      |
| CI4         | 16-color indexed texture                            | 64 × 64 indices = 2,048 bytes; palette uses TLUT space. |
| CI8         | Up to 256-color indexed texture                     | 64 × 32 indices = 2,048 bytes; palette uses TLUT space. |
| I4 or I8    | Intensity masks and compact grayscale               | 64 × 64 = 2,048 or 4,096 bytes respectively.            |
| IA8 or IA16 | Intensity plus alpha                                | 32 × 32 = 1,024 or 2,048 bytes respectively.            |

The examples are payload arithmetic, not a universal guarantee that an arbitrary upload layout fits. TMEM pitch alignment, bank layout, mipmaps, simultaneous textures and padding still matter. With indexed-color textures, the upper half of TMEM is reserved for the palette arrangement, leaving the lower half for indices. A CI4 palette containing only 16 colors does not make the rest of the upper half freely usable as ordinary texture storage.[^39]

The N64’s characteristic filtered sampling uses a three-point scheme rather than the usual four-sample bilinear interpolation. Point sampling can suit crisp artwork; filtering can hide small textures but changes edge and diagonal appearance. Test material presets with representative content rather than selecting filtering globally by taste. Mipmaps reduce distant shimmer but cost TMEM and affect the combiner path.[^40]

### Large textures and texture reuse

RDPQ can split a large 2D blit into chunks that fit TMEM. This does not remove the TMEM restriction for an arbitrary 3D triangle: the renderer must ensure the sampled region is available, typically through mesh partitioning, multiple loads or an explicit large-texture technique. Tiny3D includes a large-texture example; study its constraints before designing an entire level around large unique textures.[^41]

Build material groups that reuse a texture and compatible render state. Trim invisible image borders, consider palettes, and prefer reusable surfaces to many unique textures. Texture atlases are helpful only when their sampling and tile-load regions fit; a desktop-sized atlas is not automatically an N64 optimization. Pad atlas borders when filtering to avoid bleeding.

### Framebuffers and a proposed RAM budget

| **Configuration**         | **Color buffers** | **One 16-bit Z buffer** | **Combined payload**        |
|---------------------------|-------------------|-------------------------|-----------------------------|
| 320 × 240, 16-bit, double | 307,200 bytes     | 153,600 bytes           | 460,800 bytes = 450 KiB     |
| 320 × 240, 16-bit, triple | 460,800 bytes     | 153,600 bytes           | 614,400 bytes = 600 KiB     |
| 320 × 240, 32-bit, double | 614,400 bytes     | 153,600 bytes           | 768,000 bytes = 750 KiB     |
| 640 × 480, 16-bit, double | 1,228,800 bytes   | 614,400 bytes           | 1,843,200 bytes = 1,800 KiB |

These are calculated storage payloads: width × height × bytes per pixel × buffer count, plus the depth buffer. Alignment, display configuration and SDK allocations add overhead. A single Z buffer assumes rendering is correctly sequenced; frame-resource ownership still matters. Doubling both dimensions quadruples these storage costs and increases potential pixel work fourfold.

A starting 4 MiB planning envelope is 600 KiB for triple-buffered 16-bit color plus depth, 900 KiB for code and permanent state, 1,400 KiB for the active scene, 256 KiB for audio, 384 KiB for staging and frame arenas, and 556 KiB for SDK allocations, stacks and headroom. This is a proposed budget totaling 4,096 KiB, not a measured libdragon footprint. Replace every category with observed high-water marks before committing content.

For an Expansion Pak mode, spend extra memory on content residency, reduced streaming pressure or a deliberate visual option. Do not assume that extra RAM improves the CPU or fill rate. Keep the base-memory path exercised throughout development if supporting ordinary 4 MiB consoles is a goal.

## 10 Building the graphics engine

A useful engine boundary separates portable simulation from N64 rendering commands. The portable side owns transforms, entity IDs, collision and visibility. The N64 side owns converted assets, material state, matrix buffers, render passes and queue lifetimes. Avoid forcing a desktop GPU abstraction onto APIs that explicitly expose TMEM, fixed-point vertices and shared memory.

| **Module**           | **Owns**                                                | **Interface direction**                           |
|----------------------|---------------------------------------------------------|---------------------------------------------------|
| World and visibility | Transforms, entities, bounds, visible lists             | Produces visible object IDs and camera state.     |
| Asset registry       | Stable IDs, file paths, residency and reference counts  | Resolves IDs to loaded N64 resources.             |
| Material compiler    | RDPQ modes, textures, palette and draw class            | Produces validated material descriptions offline. |
| N64 renderer         | Tiny3D models, matrices, frame slots and command blocks | Consumes visible lists and enqueues rendering.    |
| Platform services    | Audio, input, timing, save and diagnostics              | Expose narrow engine-facing interfaces.           |

### Tiny3D geometry and animation

Tiny3D’s inspected vertex representation packs two vertices into 32 bytes. Each vertex has signed 16-bit position components, a packed normal, RGBA color and fixed-point UV data. Its vertex cache currently holds 70 transformed vertices; that is a Tiny3D implementation detail, not an immutable N64 limit. Use the supplied importer to partition and optimize meshes for the selected microcode.[^42]

Tiny3D model files use a chunked .t3dm format; animation can use companion .t3ds stream data. Keep those companion files in the build dependency graph and ROM filesystem. Treat the model converter and runtime as a versioned pair. An old asset cache can remain syntactically valid while no longer matching a changed runtime layout.[^43]

The documented skinning approach assigns one bone per vertex and supports blending between animations. It is not the usual four-weight-per-vertex desktop skinning model. Design joints, split vertices where appropriate and examine deformations on the target. Budget animation stream bandwidth and matrix updates as well as triangle count.[^44]

Recorded Tiny3D blocks can refer to matrices and vertices in RDRAM; recording does not make those resources disposable. Keep them resident and protect mutable data with the frame-resource protocol. Manual triangle submission also requires Tiny3D’s documented triangle synchronization before switching to incompatible work. The built-in model drawing path handles important details that a custom mesh format must reproduce.[^45]

### A practical frame sequence

1\. Service audio and collect input. Advance simulation with a bounded time step. Perform coarse visibility, scene streaming and animation updates. Prepare the next free frame-resource slot only after its previous work has completed.

2\. Acquire a display surface and depth buffer. Attach them, start the Tiny3D frame, configure the viewport, clear color and depth, and establish the pass state. Submit opaque material groups, then cutout and translucent content, followed by the 2D interface.

3\. Schedule presentation, record the completion conditions for resources used by the frame, and return to useful CPU work. Service audio during potentially long waits or loading phases. Avoid a global wait after every object; that defeats CPU, RSP and RDP overlap.

For exact initialization and rendering calls, start with examples/00_quad, then examples/01_model. At this snapshot the repository’s introductory text still mentions 01_quad, but the actual first example directory is 00_quad. Other focused examples cover offscreen rendering, skeletons, animation, culling, particles and mipmaps.[^46]

### Host to ROM asset pipeline

Author models in Blender and use Fast64 materials for the Tiny3D importer. Export glTF or GLB with the required custom properties, then run gltf_to_t3d. Convert source PNGs with mksprite and audio with audioconv64. The reviewed Fast64 readme lists Blender 3.2 through 5.1.2 and recommends 4.0 or newer; do not assume a newer Blender version is already supported.[^47]

The offline cooker should triangulate, apply an explicit coordinate and unit conversion, validate winding, compute bounds, select texture formats, and report per-mesh vertex counts and material costs. If importing from another engine, bake its scene data into an intermediate representation and perform N64 conversion on the host. Runtime parsing of large authoring formats wastes both code and memory.

Convert every referenced texture and preserve its expected runtime path. Prefer glTF data and Tiny3D’s own format to exporting libultra display-list C files. Fast64 supports several game-specific workflows, but original homebrew only needs its relevant material and export functionality.

## 11 Filesystems and binary formats

N64 software does not inherit a desktop filesystem. DragonFS is a read-only filesystem embedded in the ROM by mkdfs. Initialize it with dfs_init(DFS_DEFAULT_LOCATION), then use the ROM path prefix through normal C file APIs or the asset helpers. The SDK header and examples use rom:/; keep that spelling consistent even where overview documentation shows a double slash.[^48]

| **Host source**              | **Packed ROM path**             | **Runtime reference**                        |
|------------------------------|---------------------------------|----------------------------------------------|
| assets_src/textures/wall.png | filesystem/textures/wall.sprite | rom:/textures/wall.sprite                    |
| assets_src/models/room.glb   | filesystem/models/room.t3dm     | rom:/models/room.t3dm                        |
| Generated animation stream   | filesystem/models/room.t3ds     | Companion path expected by the model loader. |
| assets_src/audio/hit.wav     | filesystem/audio/hit.wav64      | rom:/audio/hit.wav64                         |
| Project-owned level data     | filesystem/levels/room.bin      | rom:/levels/room.bin                         |

These names are a proposed convention. The filesystem/ directory itself is not part of the runtime path. On the host, all nested conversions must create their output directories. A file copied next to game.z64 is not automatically inside the ROM. Include the .dfs image as a prerequisite of the final ROM target and the converted assets as prerequisites of that image.[^49]

### Adding DragonFS to the build

``` makefile
# Add after the SDK include in the project Makefile.
# ASSET_FILES must list the actual generated files.
ASSET_FILES := filesystem/levels/room.bin
$(BUILD_DIR)/game.dfs: $(ASSET_FILES)
game.z64: $(BUILD_DIR)/game.dfs
```

A production build should derive ASSET_FILES from a manifest or explicit conversion rules. Detect added and deleted files as well as modified files. Avoid a stale .dfs after an asset rename. Names that differ only by case are especially troublesome when a project moves between Windows and Linux hosts.

The inspected DragonFS contract allows four files open at once, does not support writes or symlinks, and rejects empty files and directories. Close handles promptly, especially when audio and animation also stream. Its maximum individual-file field size is not a promise that a cartridge or physical ROM can hold a file that large.[^50]

### Compression and serialization

asset_load loads and optionally decompresses a whole asset; asset_fopen provides sequential decompression through a FILE interface. The documented asset_fopen contract rejects seeking even for uncompressed data, preserving the ability to compress later. Use ordinary fopen only when random access to an explicitly uncompressed file is required. Enable any non-default compression level before using assets encoded with it.[^51]

Choose chunk sizes according to residency and latency. A compressed 2 MiB level does not require only its compressed size in memory once expanded. Account for decoded output, source staging, decoder state and temporary scene data at the same time. Avoid recompressing formats that already have an intentional streaming or codec structure without checking loader support.

For project-owned binary data, define a magic value, format version, byte order, section lengths and offsets. Store integers in a chosen explicit endian format and serialize fields individually. Validate offset-plus-length arithmetic against file size before reading. Align payloads for the target consumer and prefer relative offsets over host pointers. Use IDs to connect entities and assets; resolve them after loading.

### Dynamic code and optional storage

libdragon supports dynamically loaded code through its DSO mechanism and dlopen or dlsym. This can reduce resident code or support modular content, but it adds relocation, lifetime and dependency management. Load code through the SDK rather than memcpy followed by a function-pointer cast; instruction-cache coherency matters too. Introduce DSOs only when the memory map shows a concrete benefit.[^52]

Some development cartridges expose SD-card access through libdragon. Keep it as a capability of the cartridge-backed platform service, not a universal property of the console. A retail-style standalone ROM should not accidentally depend on a host path, SD filesystem or USB cable. The same distinction applies to flashcart-specific RTC and networking extensions.[^53]

## 12 Audio input and persistent saves

### Audio architecture

The audio interface consumes PCM buffers; music sequencing, decoding, mixing and resampling are software responsibilities. libdragon’s audio layer queues buffers for output, while its mixer uses the RSP and supports up to 32 channels. The output callback’s sample count is per channel, with interleaved stereo output, so buffer sizing must account for both channels.[^54]

Convert sound effects to wav64 using audioconv64. For music, evaluate streamed audio or module playback according to fidelity, memory, CPU and RSP cost. In the inspected preview snapshot, examples use wav64_load and mixer_try_play. Do not mix these calls with an older tutorial’s ownership or streaming model without checking the corresponding header.[^55]

Treat audio as a deadline-driven service. Call the appropriate mixer service during the normal loop and long loading operations, or use a supported callback integration. A game with a stable average frame rate can still click if it stalls for one long asset transfer. More buffered audio improves tolerance to stalls but increases input-to-sound latency.

Use a small initial channel budget, for example 8–16 voices, and scale only after measuring the combined graphics and audio workload. Choose sample rates and compression by listening on the actual output path. The SDK handles regional timing when selecting audio hardware rates; query the actual configured frequency when that distinction matters.[^56]

### Controllers and accessories

Use joypad_init once and joypad_poll once per frame to process sampled input. Distinguish pressed, released and held states; use pressed for menu actions and held for continuous movement. The API supports the four ports and exposes controller style and accessory capability. Do not assume a controller or Rumble Pak is always connected.[^57]

Apply a tunable analog dead zone, then normalize and clamp into an engine-facing range. Preserve useful analog magnitude and handle diagonal input consistently. Test original, worn and replacement controllers. Choose behavior for disconnect and reconnect, including whether a missing player pauses the game. Accessory detection should precede rumble or Controller Pak operations.

### Save media and recovery

| **Medium**                 | **Capacity reference**                                   | **Design consequence**                                                     |
|----------------------------|----------------------------------------------------------|----------------------------------------------------------------------------|
| EEPROM 4 kbit              | 512 bytes raw                                            | Suitable for settings and small progress records.                          |
| EEPROM 16 kbit             | 2,048 bytes raw                                          | More progress data, still a very small schema.                             |
| EEPROM filesystem          | 504 or 2,040 bytes before optional record overhead       | Reserved metadata and backup or checksum features reduce payload capacity. |
| Controller Pak             | Organized as 256-byte pages with filesystem overhead     | Removable storage; handle full, missing or corrupt media.                  |
| Cartridge SRAM or FlashRAM | Depends on the selected cartridge hardware and save type | Declare the intended save type and use its matching API.                   |

The labels 4k and 16k for EEPROM are bits, not kilobytes. Set N64_ROM_SAVETYPE in the ROM build metadata and probe available storage. EEPROM writes may complete in the background in the reviewed preview API; wait for the documented completion condition before reporting durable success or permitting a reset-sensitive transition.[^58]

Define a versioned save record with a checksum, generation number and payload length. Where capacity permits, keep two slots and write the new generation before invalidating the old one. A checksum detects corruption but does not alone make an interrupted write atomic. Test missing media, full storage, unexpected reset and loading an older save schema.

Keep persistence APIs separate from game-state structs. A stable serialized format makes migrations possible and avoids recording pointers or compiler padding. Flashcart emulation of save media is useful for testing, but also verify any reset or SD writeback behavior of the actual cartridge used for distribution.[^59]

## 13 Timing profiling and optimization

The CPU clock gives an upper accounting envelope, not an instruction budget. At nominal 60 frames per second there are 1,562,500 CPU clock cycles per frame; at 30 there are 3,125,000. Cache misses, pipeline dependencies, interrupts and shared-memory contention consume elapsed time. RSP, RDP and VI run independently enough that adding their individual timings is not a correct frame-time model.

libdragon’s counter ticks at half the CPU frequency. On an ordinary N64 that is 46,875,000 ticks per second. Use the documented conversion and wrap-safe distance helpers, or the longer-range timer interface, rather than treating raw ticks as microseconds. For a nominal 60 Hz workload, the frame interval is roughly 16.7 ms; PAL or variable presentation needs separate handling.[^60]

### Measure the bottleneck

| **Observation**                       | **Likely next experiment**                                                                        |
|---------------------------------------|---------------------------------------------------------------------------------------------------|
| CPU time grows with entities          | Profile visibility, transforms and collision; inspect data locality before adding assembly.       |
| Geometry changes dominate             | Compare fixed camera, lower mesh detail, fewer lights and simpler animation.                      |
| Resolution strongly changes speed     | Investigate RDP pixel work, blending, framebuffer traffic and fill cost.                          |
| Material count strongly changes speed | Reduce state changes and texture uploads; group compatible surfaces.                              |
| Audio glitches during loads           | Measure worst-case service gaps and transfer bursts, not only average FPS.                        |
| Long sessions degrade                 | Track allocations, peak memory, fragmentation and resource retirement.                            |
| Only hardware is unstable             | Review DMA alignment, coherency, timing, unsupported emulator behavior and cartridge assumptions. |

Instrument CPU sections with timers and use libdragon’s RSPQ profiling and debug facilities for the coprocessor work. Separate CPU submission time from completion time. Capture statistics over representative scenes, including outliers, not only an empty demo. An emulator is excellent for diagnosis, but final performance decisions need hardware measurements.[^61]

There is no single reliable polygons-per-second figure for an engine. Transformed vertices, clipped triangles, screen area, texture traffic, material state, blending and audio all change the result. Benchmark a content matrix: opaque textured scene, many skinned objects, fill-heavy effects, heavy UI, and simultaneous streaming. Set budgets from the slowest acceptable case.

### Optimization order

First remove invisible work through frustum culling, spatial partitioning and appropriate level of detail. Next reduce memory traffic and redundant state changes. Then precompute static data, record reusable command blocks, and improve batch structure. Only then specialize arithmetic or write custom microcode. Keep a correctness reference so numerical shortcuts cannot silently damage the renderer.

Tiny3D’s importer reorders and partitions geometry to reuse its vertex cache and reduce command traffic. It may duplicate a small amount of vertex data to make transfers more regular. This is a useful example of spending ROM or RAM capacity to reduce runtime work; minimum file size and maximum render speed are not always the same objective.[^62]

Avoid unbounded work inside one frame. Break asset loads into chunks, cap catch-up simulation after a stall, and keep audio service available. Use heap allocation mainly during transitions; fixed pools or arenas make frame-time behavior easier to predict. Test transitions between the two largest scenes because both old and new data may coexist temporarily.

## 14 Debugging and hardware deployment

Use ares as the primary emulator for this toolchain and enable its Homebrew mode. Accurate low-level execution matters because libdragon and Tiny3D use custom microcode and hardware behavior that an emulator optimized for familiar commercial games may not reproduce. Retain real-hardware tests from the first ROM onward; emulator success is strong evidence, not a hardware certification.[^63]

### Logs and rendering validation

Initialize the supported debug output early. debugf sends diagnostics through the configured emulator or USB logging path. Enable rdpq_debug_start after rdpq_init when diagnosing state or command problems. The RDPQ tracer and validator can identify invalid rendering state that would otherwise appear as a black screen, corruption or a hang.[^64]

Keep logs bounded so logging does not become the performance problem. On a crash, retain the exact ROM hash, ELF, scene and reproduction steps. Symbolized traces are more useful than a screenshot of a bare address. For memory bugs, log ownership transitions and allocation sizes, not just the last function reached.

### GDB workflow

``` bash
# Build with debug information; this does not imply -O0.
make D=1
# Start the ROM in ares with Homebrew and GDB enabled.
# Then load symbols from the ELF, not the .z64:
mips64-elf-gdb build/game.elf

# In GDB, matching the address shown by ares:
target remote 127.0.0.1:9123
break main
continue
bt
```

The documented ares example uses port 9123 with IPv4 enabled. Use the actual address displayed by the emulator. Set the emulator to wait for the debugger when you need to catch startup. If symbols or source lines do not resolve, check the GDB version and source-path mapping against the exact build, especially across container, WSL and host paths.[^65]

Optimized code may step in surprising order or optimize variables away. D=1 adds debug information in n64.mk; it does not automatically disable optimization. Change optimization deliberately for an investigation and reproduce the fix in the normal optimized build before closing the issue.[^66]

### Development cartridge workflow

SummerCart64 is a suitable open development option because it provides USB upload and debug support as well as SD loading and save emulation. UNFLoader is another established upload and logging tool for supported cartridges. These are development transport choices; their software and firmware licenses are separate from the ROM’s runtime dependencies.[^67]

A useful loop is build, retain ELF and hash, upload the same ROM, run the same scene, capture logs, then cold-boot it from standalone storage. Test warm reset as well as power-on. Confirm operation with USB disconnected so a development-only host dependency cannot slip into the release.

### Failure patterns

| **Symptom**                         | **First checks**                                                                                    |
|-------------------------------------|-----------------------------------------------------------------------------------------------------|
| Black screen before any log         | Correct ROM and bootloader, current emulator, memory requirement, linker payload and early startup. |
| File opens fail                     | dfs_init, .dfs target dependency, rom:/ path, letter case, converted output and file-handle limit.  |
| Texture corruption                  | TMEM fit, palette state, format, pitch, UV range, missing sync or freed source memory.              |
| Geometry flickers or explodes       | Matrix or vertex lifetime, alignment, fixed-point conversion, cache maintenance and winding.        |
| RSP timeout                         | Overlay compatibility, invalid command or DMA, missing completion, manual microcode ownership.      |
| Frame stutter with good average FPS | Outlier loads, queue stalls, allocator behavior, buffer reuse and presentation scheduling.          |
| Save disappears after reset         | Declared save type, completion before reset, cartridge writeback and recovery logic.                |
| Works only with Expansion Pak       | Actual memory use, static payload size, accidental detection assumptions or fragmentation.          |

## 15 Implementation plan and release criteria

The following order produces useful checkpoints without requiring the entire engine to exist first. Each milestone should remain runnable as a small diagnostic scene.

| **Stage**              | **Deliverable**                             | **Acceptance evidence**                                                       |
|------------------------|---------------------------------------------|-------------------------------------------------------------------------------|
| 1 Platform bootstrap   | First C++ ROM, display and logs             | Same ROM starts in ares and on a 4 MiB console.                               |
| 2 Asset path           | One packed texture and one data file        | Repeatable conversion; missing or renamed input causes a clear build failure. |
| 3 Basic 3D             | Tiny3D camera, textured model and depth     | Correct near-plane behavior, winding, material state and resource lifetimes.  |
| 4 Services             | Input, sound and persistence                | Disconnect handling, no audio starvation, save survives cold boot.            |
| 5 Scene architecture   | Visibility, asset residency and transitions | Bounded peak memory and no use-after-free during transitions.                 |
| 6 Production rendering | Animation, UI, particles and materials      | Worst-case scene meets the chosen frame and audio deadlines.                  |
| 7 Packaging            | Standalone ROM and license notices          | Clean rebuild, recorded revisions, asset provenance and hardware test matrix. |

Keep a base configuration that is easy to measure: 320 × 240, 16-bit color, modest material variety and a limited number of audio voices. Add resolution, effects and content density independently so each cost remains visible. If development speed matters more than engine ownership, evaluate Pyrite64 before building an editor, scene serializer and asset registry from scratch.

### Release record

Record the ROM hash, source revisions, compiler identity, linked dependencies, save type, memory requirement, supported input devices and TV modes. Include applicable license notices and original-asset provenance. Retain the ELF and map privately or with the source release so later crash reports remain actionable.

Verify cold boot, warm reset, standalone launch, representative PAL and NTSC timing, optional MPAL support if claimed, controller loss, absent or corrupt save media, worst-case scene transitions and the minimum memory configuration. If only a subset was tested, describe that subset accurately. Region-free metadata and emulator compatibility are not replacements for this evidence.

### Peripheral scope

The base design targets a normal cartridge program on an N64. Controller Pak, Rumble Pak and cartridge save hardware are covered through the SDK’s abstractions. Transfer Pak, special controllers, 64DD and iQue require separate capability checks and focused testing. SummerCart64’s 64DD emulation or RTC does not make those capabilities part of every N64.[^68]

Similarly, a USB-debug connection is not a console-wide networking API. Put optional transports behind interfaces and keep the standalone game behavior well-defined without them. Commercial-ROM compatibility, retail microcode emulation and the reproduction of proprietary SDK behavior are outside this engine’s requirements.

## 16 Source path navigator

Paths below are relative to the upstream repository, not your installed SDK root. They are a reading order for going deeper when the public API stops being sufficient. Internal files explain behavior but should not become dependencies merely because they are visible.

| **Question**                              | **libdragon source path**                                                |
|-------------------------------------------|--------------------------------------------------------------------------|
| What actually builds and links a ROM      | n64.mk; n64.ld; tools/n64tool.c; boot/README.md                          |
| Which compiler and ABI are used           | tools/build-toolchain.sh; n64.mk                                         |
| How addresses and caches work             | include/n64sys.h; src/n64sys.c                                           |
| How PI transfers and ROM access work      | include/dma.h; src/dma.c                                                 |
| How files and compressed assets load      | include/dragonfs.h; include/asset.h; src/asset.c                         |
| How display surfaces are owned            | include/display.h; include/surface.h; include/rdpq_attach.h              |
| How RDP modes and textures are configured | include/rdpq.h; include/rdpq_mode.h; include/rdpq_tex.h                  |
| How RSP jobs coexist                      | include/rsp.h; include/rspq.h; src/rspq/                                 |
| How to trace and profile rendering        | include/rdpq_debug.h; include/rspq_profile.h                             |
| How audio is produced                     | include/audio.h; include/mixer.h; include/wav64.h                        |
| How input and saves work                  | include/joypad.h; include/eeprom.h; include/eepromfs.h; include/mempak.h |

| **Question**                          | **Tiny3D source path**                                           |
|---------------------------------------|------------------------------------------------------------------|
| Start with the smallest 3D program    | examples/00_quad/main.c                                          |
| Load a converted model                | examples/01_model/; tools/gltf_importer/                         |
| Understand packed vertices and APIs   | src/t3d/t3d.h; src/t3d/t3dmodel.h                                |
| Understand file layout and conversion | docs/modelFormat.md; docs/modelOpt.md; docs/fast64Settings.md    |
| Study skeletal motion                 | examples/07_skeleton/; examples/08_animation/; src/t3d/t3danim.h |
| Study performance techniques          | examples/17_culling/; examples/20_mipmaps/; examples/22_bigtex/  |
| Inspect or modify microcode           | src/t3d/rsp/; generated .S and source .rspl files                |
| Integrate local or installed builds   | t3d.mk; t3d-inst.mk; build.sh                                    |

The best long-term reference is the header and example from the same revision as the binary. Wiki prose can lag source: the first Tiny3D example directory and several preview APIs already demonstrate why mixing revisions is risky. Treat proposed budgets and engine policies here as starting decisions, then replace them with measurements from the actual game.

## Sources

Numbered footnotes link the claims to their original sources. This inventory includes the source files actually used, with pinned repository links where a source snapshot was inspected. Living documentation is identified by access date. Primary project documentation and implementation sources take precedence over third-party setup tutorials.

1\. libdragon contributors. [libdragon features and branch policy](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/README.md). Source snapshot 15 September 2026.

2\. Max Bebök and Tiny3D contributors. [Tiny3D overview and integration](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/README.md). Source snapshot 30 August 2026.

3\. libdragon contributors. [libdragon license](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/LICENSE.md). Source snapshot 15 September 2026.

4\. Max Bebök and Tiny3D contributors. [Tiny3D MIT license](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/LICENSE). Source snapshot 30 August 2026.

5\. Free Software Foundation. [GNU GPL FAQ on tools and generated output](https://www.gnu.org/licenses/gpl-faq.en.html#CanIUseGPLToolsForNF). Accessed 15 September 2026.

6\. Free Software Foundation. [GCC runtime library licensing](https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html). GCC Runtime Library Exception 3.1, 31 March 2009.

7\. libdragon contributors. [n64tool source and GPL notice](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/tools/n64tool.c). Source snapshot 15 September 2026.

8\. Newlib contributors. [Newlib copyright and license collection](https://sourceware.org/newlib/COPYING.NEWLIB). Accessed 15 September 2026.

9\. libdragon contributors. [Open source IPL3 bootloader](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/boot/README.md). Source snapshot 15 September 2026.

10\. libdragon contributors. [Boot font provenance](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/boot/assets/README.md). Source snapshot 15 September 2026.

11\. 1001Fonts. [1001Fonts Free For Commercial Use license](https://www.1001fonts.com/licenses/ffc.html). Accessed 15 September 2026.

12\. Giovanni Bajo and libdragon contributors. [OpenGL on N64](https://github.com/DragonMinded/libdragon/wiki/OpenGL-on-N64). Wiki revision 1 October 2024; accessed 15 September 2026.

13\. Max Bebök and Pyrite64 contributors. [Pyrite64 readme and licensing](https://github.com/HailToDodongo/pyrite64). Accessed 15 September 2026.

14\. libdragon contributors. [Installing libdragon](https://github.com/DragonMinded/libdragon/wiki/Installing-libdragon). Accessed 15 September 2026.

15\. libdragon contributors. [tools/build-toolchain.sh](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/tools/build-toolchain.sh). Source snapshot 15 September 2026.

16\. libdragon contributors. [n64.mk](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.mk). Source snapshot 15 September 2026.

17\. Max Bebök and Tiny3D contributors. [t3d.mk](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/t3d.mk). Source snapshot 30 August 2026.

18\. Max Bebök and Tiny3D contributors. [t3d-inst.mk](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/t3d-inst.mk). Source snapshot 30 August 2026.

19\. libdragon contributors. [include/display.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/display.h). Source snapshot 15 September 2026.

20\. libdragon contributors. [include/rdpq_attach.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_attach.h). Source snapshot 15 September 2026.

21\. libdragon contributors. [include/rdpq_mode.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_mode.h). Source snapshot 15 September 2026.

22\. libdragon contributors. [include/n64sys.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/n64sys.h). Source snapshot 15 September 2026.

23\. libdragon contributors. [src/n64sys.c](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/src/n64sys.c). Source snapshot 15 September 2026.

24\. n64docs author and contributors. [n64docs hardware overview and memory map](https://n64.readthedocs.io/). Undated community implementation notes; accessed 15 September 2026.

25\. ares contributors. [ares RSP implementation and register layout](https://github.com/ares-emulator/ares/blob/master/ares/n64/rsp/rsp.hpp). Accessed 15 September 2026.

26\. libdragon contributors. [include/regsinternal.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/regsinternal.h). Source snapshot 15 September 2026.

27\. libdragon contributors. [include/dma.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/dma.h). Source snapshot 15 September 2026.

28\. libdragon contributors. [include/rsp.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rsp.h). Source snapshot 15 September 2026.

29\. libdragon contributors. [n64.ld](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.ld). Source snapshot 15 September 2026.

30\. Max Bebök and Tiny3D contributors. [docs/modelFormat.md](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/docs/modelFormat.md). Source snapshot 30 August 2026.

31\. libdragon contributors. [include/interrupt.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/interrupt.h). Source snapshot 15 September 2026.

32\. libdragon contributors. [include/rspq.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rspq.h). Source snapshot 15 September 2026.

33\. Max Bebök and RSPL contributors. [RSPL language and licensing](https://github.com/HailToDodongo/rspl). Accessed 15 September 2026.

34\. libdragon contributors. [include/rdpq.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq.h). Source snapshot 15 September 2026.

35\. libdragon contributors. [RDP triangle attributes and perspective texturing](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_tri.h). Source snapshot 15 September 2026.

36\. Max Bebök and Tiny3D contributors. [src/t3d/t3d.h](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/src/t3d/t3d.h). Source snapshot 30 August 2026.

37\. libdragon contributors. [PAL NTSC support](https://github.com/DragonMinded/libdragon/wiki/PAL-NTSC-support). Accessed 15 September 2026.

38\. libdragon contributors. [src/rdpq/rdpq_tex.c](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/src/rdpq/rdpq_tex.c). Source snapshot 15 September 2026.

39\. libdragon contributors. [include/surface.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/surface.h). Source snapshot 15 September 2026.

40\. libdragon contributors. [include/rdpq_tex.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_tex.h). Source snapshot 15 September 2026.

41\. Hans-Kristian Arntzen and paraLLEl-RDP contributors. [RDP texture filtering implementation](https://github.com/Themaister/parallel-rdp/blob/1cecd042b2619bc505c12bfdc713808386f2b54d/parallel-rdp/shaders/texture.h). Source snapshot 9 November 2024.

42\. Max Bebök and Tiny3D contributors. [docs/modelOpt.md](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/docs/modelOpt.md). Source snapshot 30 August 2026.

43\. Max Bebök and Tiny3D contributors. [src/t3d/t3danim.h](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/src/t3d/t3danim.h). Source snapshot 30 August 2026.

44\. Max Bebök and Tiny3D contributors. [examples/00_quad/main.c](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/examples/00_quad/main.c). Source snapshot 30 August 2026.

45\. Max Bebök and Tiny3D contributors. [build.sh](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/build.sh). Source snapshot 30 August 2026.

46\. Fast64 contributors. [Fast64 readme and license declaration](https://github.com/Fast-64/fast64). Accessed 15 September 2026.

47\. Max Bebök and Tiny3D contributors. [docs/fast64Settings.md](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/docs/fast64Settings.md). Source snapshot 30 August 2026.

48\. Max Bebök and Tiny3D contributors. [examples/01_model/Makefile](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/examples/01_model/Makefile). Source snapshot 30 August 2026.

49\. libdragon contributors. [include/dragonfs.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/dragonfs.h). Source snapshot 15 September 2026.

50\. libdragon contributors. [include/asset.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/asset.h). Source snapshot 15 September 2026.

51\. libdragon contributors. [DSO dynamic libraries](https://github.com/DragonMinded/libdragon/wiki/DSO-(dynamic-libraries)). Accessed 15 September 2026.

52\. libdragon contributors. [src/dlfcn.c](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/src/dlfcn.c). Source snapshot 15 September 2026.

53\. SummerCart64 contributors. [SummerCart64 features and documentation](https://github.com/Polprzewodnikowy/SummerCart64). Accessed 15 September 2026.

54\. libdragon contributors. [include/audio.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/audio.h). Source snapshot 15 September 2026.

55\. libdragon contributors. [include/mixer.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/mixer.h). Source snapshot 15 September 2026.

56\. libdragon contributors. [include/wav64.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/wav64.h). Source snapshot 15 September 2026.

57\. libdragon contributors. [include/joypad.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/joypad.h). Source snapshot 15 September 2026.

58\. libdragon contributors. [include/eeprom.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/eeprom.h). Source snapshot 15 September 2026.

59\. libdragon contributors. [include/eepromfs.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/eepromfs.h). Source snapshot 15 September 2026.

60\. libdragon contributors. [include/mempak.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/mempak.h). Source snapshot 15 September 2026.

61\. libdragon contributors. [include/rspq_profile.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rspq_profile.h). Source snapshot 15 September 2026.

62\. libdragon contributors. [include/debug.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/debug.h). Source snapshot 15 September 2026.

63\. ares contributors. [ares official website](https://ares-emu.net/). Accessed 15 September 2026.

64\. libdragon contributors. [include/rdpq_debug.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_debug.h). Source snapshot 15 September 2026.

65\. Nadia Holmquist Pedersen and libdragon contributors. [Debugging via gdb](https://github.com/DragonMinded/libdragon/wiki/Debugging-via-gdb). Wiki revision 24 July 2026.

66\. buu342 and contributors. [N64 UNFLoader](https://github.com/buu342/N64-UNFLoader). Accessed 15 September 2026.

67\. ares contributors. [ares license collection](https://github.com/ares-emulator/ares/blob/master/LICENSE). Accessed 15 September 2026.

[^1]: libdragon contributors, [libdragon features and branch policy](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/README.md); Max Bebök and Tiny3D contributors, [Tiny3D overview and integration](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/README.md).

[^2]: libdragon contributors, [libdragon license](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/LICENSE.md); Max Bebök and Tiny3D contributors, [Tiny3D MIT license](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/LICENSE); Max Bebök and Tiny3D contributors, [Tiny3D overview and integration](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/README.md).

[^3]: Free Software Foundation, [GNU GPL FAQ on tools and generated output](https://www.gnu.org/licenses/gpl-faq.en.html#CanIUseGPLToolsForNF); Free Software Foundation, [GCC runtime library licensing](https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html); libdragon contributors, [n64tool source and GPL notice](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/tools/n64tool.c); Newlib contributors, [Newlib copyright and license collection](https://sourceware.org/newlib/COPYING.NEWLIB).

[^4]: libdragon contributors, [Open source IPL3 bootloader](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/boot/README.md).

[^5]: libdragon contributors, [Boot font provenance](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/boot/assets/README.md); 1001Fonts, [1001Fonts Free For Commercial Use license](https://www.1001fonts.com/licenses/ffc.html).

[^6]: Giovanni Bajo and libdragon contributors, [OpenGL on N64](https://github.com/DragonMinded/libdragon/wiki/OpenGL-on-N64).

[^7]: Max Bebök and Pyrite64 contributors, [Pyrite64 readme and licensing](https://github.com/HailToDodongo/pyrite64).

[^8]: libdragon contributors, [libdragon features and branch policy](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/README.md); Max Bebök and Tiny3D contributors, [Tiny3D overview and integration](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/README.md).

[^9]: libdragon contributors, [Installing libdragon](https://github.com/DragonMinded/libdragon/wiki/Installing-libdragon).

[^10]: libdragon contributors, [tools/build-toolchain.sh](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/tools/build-toolchain.sh); libdragon contributors, [n64.mk](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.mk).

[^11]: libdragon contributors, [n64.mk](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.mk); Max Bebök and Tiny3D contributors, [t3d.mk](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/t3d.mk); Max Bebök and Tiny3D contributors, [t3d-inst.mk](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/t3d-inst.mk).

[^12]: libdragon contributors, [include/display.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/display.h); libdragon contributors, [include/rdpq_attach.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_attach.h); libdragon contributors, [include/rdpq_mode.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_mode.h).

[^13]: libdragon contributors, [n64.mk](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.mk).

[^14]: libdragon contributors, [include/n64sys.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/n64sys.h); libdragon contributors, [src/n64sys.c](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/src/n64sys.c); n64docs author and contributors, [n64docs hardware overview and memory map](https://n64.readthedocs.io/); ares contributors, [ares RSP implementation and register layout](https://github.com/ares-emulator/ares/blob/master/ares/n64/rsp/rsp.hpp).

[^15]: libdragon contributors, [include/regsinternal.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/regsinternal.h); libdragon contributors, [include/dma.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/dma.h); libdragon contributors, [include/rsp.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rsp.h); n64docs author and contributors, [n64docs hardware overview and memory map](https://n64.readthedocs.io/).

[^16]: libdragon contributors, [include/n64sys.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/n64sys.h).

[^17]: libdragon contributors, [Open source IPL3 bootloader](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/boot/README.md); libdragon contributors, [n64.ld](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.ld).

[^18]: libdragon contributors, [n64.mk](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.mk).

[^19]: libdragon contributors, [tools/build-toolchain.sh](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/tools/build-toolchain.sh); libdragon contributors, [n64.mk](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.mk).

[^20]: libdragon contributors, [n64.mk](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.mk); libdragon contributors, [n64.ld](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.ld); Max Bebök and Tiny3D contributors, [docs/modelFormat.md](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/docs/modelFormat.md).

[^21]: libdragon contributors, [n64.mk](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.mk).

[^22]: libdragon contributors, [tools/build-toolchain.sh](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/tools/build-toolchain.sh).

[^23]: libdragon contributors, [include/interrupt.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/interrupt.h); libdragon contributors, [libdragon features and branch policy](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/README.md).

[^24]: libdragon contributors, [include/n64sys.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/n64sys.h).

[^25]: libdragon contributors, [src/n64sys.c](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/src/n64sys.c); libdragon contributors, [include/n64sys.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/n64sys.h); libdragon contributors, [include/dma.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/dma.h).

[^26]: libdragon contributors, [include/dma.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/dma.h).

[^27]: libdragon contributors, [include/rspq.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rspq.h); libdragon contributors, [include/rdpq_attach.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_attach.h).

[^28]: ares contributors, [ares RSP implementation and register layout](https://github.com/ares-emulator/ares/blob/master/ares/n64/rsp/rsp.hpp); libdragon contributors, [include/rsp.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rsp.h).

[^29]: libdragon contributors, [include/rspq.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rspq.h).

[^30]: libdragon contributors, [include/rsp.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rsp.h); libdragon contributors, [n64.mk](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.mk).

[^31]: Max Bebök and RSPL contributors, [RSPL language and licensing](https://github.com/HailToDodongo/rspl); Max Bebök and Tiny3D contributors, [Tiny3D overview and integration](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/README.md).

[^32]: libdragon contributors, [include/rdpq.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq.h).

[^33]: libdragon contributors, [RDP triangle attributes and perspective texturing](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_tri.h); Max Bebök and Tiny3D contributors, [src/t3d/t3d.h](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/src/t3d/t3d.h).

[^34]: libdragon contributors, [include/rdpq_mode.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_mode.h).

[^35]: libdragon contributors, [include/rdpq_attach.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_attach.h).

[^36]: libdragon contributors, [include/display.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/display.h).

[^37]: libdragon contributors, [PAL NTSC support](https://github.com/DragonMinded/libdragon/wiki/PAL-NTSC-support).

[^38]: libdragon contributors, [src/rdpq/rdpq_tex.c](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/src/rdpq/rdpq_tex.c); libdragon contributors, [include/rdpq.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq.h).

[^39]: libdragon contributors, [include/surface.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/surface.h); libdragon contributors, [include/rdpq_tex.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_tex.h); libdragon contributors, [src/rdpq/rdpq_tex.c](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/src/rdpq/rdpq_tex.c).

[^40]: Hans-Kristian Arntzen and paraLLEl-RDP contributors, [RDP texture filtering implementation](https://github.com/Themaister/parallel-rdp/blob/1cecd042b2619bc505c12bfdc713808386f2b54d/parallel-rdp/shaders/texture.h); libdragon contributors, [include/rdpq_mode.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_mode.h).

[^41]: libdragon contributors, [include/rdpq_tex.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_tex.h); Max Bebök and Tiny3D contributors, [Tiny3D overview and integration](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/README.md).

[^42]: Max Bebök and Tiny3D contributors, [src/t3d/t3d.h](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/src/t3d/t3d.h); Max Bebök and Tiny3D contributors, [docs/modelOpt.md](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/docs/modelOpt.md).

[^43]: Max Bebök and Tiny3D contributors, [docs/modelFormat.md](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/docs/modelFormat.md); Max Bebök and Tiny3D contributors, [src/t3d/t3danim.h](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/src/t3d/t3danim.h).

[^44]: Max Bebök and Tiny3D contributors, [Tiny3D overview and integration](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/README.md).

[^45]: Max Bebök and Tiny3D contributors, [examples/00_quad/main.c](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/examples/00_quad/main.c); Max Bebök and Tiny3D contributors, [src/t3d/t3d.h](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/src/t3d/t3d.h).

[^46]: Max Bebök and Tiny3D contributors, [examples/00_quad/main.c](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/examples/00_quad/main.c); Max Bebök and Tiny3D contributors, [build.sh](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/build.sh).

[^47]: Fast64 contributors, [Fast64 readme and license declaration](https://github.com/Fast-64/fast64); Max Bebök and Tiny3D contributors, [docs/fast64Settings.md](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/docs/fast64Settings.md); Max Bebök and Tiny3D contributors, [examples/01_model/Makefile](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/examples/01_model/Makefile).

[^48]: libdragon contributors, [include/dragonfs.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/dragonfs.h).

[^49]: libdragon contributors, [n64.mk](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.mk); Max Bebök and Tiny3D contributors, [examples/01_model/Makefile](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/examples/01_model/Makefile).

[^50]: libdragon contributors, [include/dragonfs.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/dragonfs.h).

[^51]: libdragon contributors, [include/asset.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/asset.h).

[^52]: libdragon contributors, [DSO dynamic libraries](https://github.com/DragonMinded/libdragon/wiki/DSO-(dynamic-libraries)); libdragon contributors, [src/dlfcn.c](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/src/dlfcn.c).

[^53]: libdragon contributors, [libdragon features and branch policy](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/README.md); SummerCart64 contributors, [SummerCart64 features and documentation](https://github.com/Polprzewodnikowy/SummerCart64).

[^54]: libdragon contributors, [include/audio.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/audio.h); libdragon contributors, [include/mixer.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/mixer.h).

[^55]: libdragon contributors, [include/wav64.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/wav64.h).

[^56]: libdragon contributors, [include/audio.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/audio.h).

[^57]: libdragon contributors, [include/joypad.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/joypad.h).

[^58]: libdragon contributors, [include/eeprom.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/eeprom.h); libdragon contributors, [include/eepromfs.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/eepromfs.h); libdragon contributors, [include/mempak.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/mempak.h); libdragon contributors, [n64.mk](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.mk).

[^59]: SummerCart64 contributors, [SummerCart64 features and documentation](https://github.com/Polprzewodnikowy/SummerCart64).

[^60]: libdragon contributors, [include/n64sys.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/n64sys.h).

[^61]: libdragon contributors, [include/rspq_profile.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rspq_profile.h); libdragon contributors, [include/debug.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/debug.h).

[^62]: Max Bebök and Tiny3D contributors, [docs/modelOpt.md](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/docs/modelOpt.md).

[^63]: libdragon contributors, [libdragon features and branch policy](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/README.md); ares contributors, [ares official website](https://ares-emu.net/).

[^64]: libdragon contributors, [include/debug.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/debug.h); libdragon contributors, [include/rdpq_debug.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_debug.h).

[^65]: Nadia Holmquist Pedersen and libdragon contributors, [Debugging via gdb](https://github.com/DragonMinded/libdragon/wiki/Debugging-via-gdb).

[^66]: libdragon contributors, [n64.mk](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/n64.mk).

[^67]: SummerCart64 contributors, [SummerCart64 features and documentation](https://github.com/Polprzewodnikowy/SummerCart64); buu342 and contributors, [N64 UNFLoader](https://github.com/buu342/N64-UNFLoader); ares contributors, [ares license collection](https://github.com/ares-emulator/ares/blob/master/LICENSE).

[^68]: libdragon contributors, [include/joypad.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/joypad.h); SummerCart64 contributors, [SummerCart64 features and documentation](https://github.com/Polprzewodnikowy/SummerCart64).
