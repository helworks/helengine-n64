# Nintendo 64 Rendering Optimization

The best starting point for a new performance-focused N64 renderer is **libdragon RDPQ plus Tiny3D**, with visibility, mesh conversion, and material organization designed around the hardware. Tiny3D supplies independently written RSP microcode, a C API, mesh preprocessing, and integration with RDPQ. This is a recommended baseline, not evidence that it wins every workload against every alternative.[^1][^2]

The largest repeatable gains usually come from eliminating work before it enters the pipeline, reducing command and texture traffic, and allowing processors to overlap. Replacing the microcode becomes attractive after measurements identify geometry processing as the remaining limit.

## The pipeline and its bottlenecks

| Stage | Work to account for | First optimization candidates |
|---|---|---|
| CPU | Simulation, visibility, animation, sorting, command submission | Cull whole objects; retain commands; reduce rebuilding |
| RSP | Vertex transforms, lighting, clipping, triangle setup; also audio | Reuse vertices; simplify lights; reduce clips; specialize batches |
| RDP | Rasterization, texture sampling, depth, coverage, blending | Reduce drawn pixels, state changes, overdraw and costly modes |
| Shared memory and queues | Vertex and command transfers, texture loads, framebuffer traffic | Improve locality; reduce bytes; avoid waits and buffer hazards |

The RDP consumes rasterization commands; it does not perform model transforms or lighting. RDPQ routes work through the RSP, and its asynchronous queues are central to CPU/RSP/RDP overlap. A slow frame can be limited by any stage or by back-pressure between them.[^2][^24]

## Recommended order of work

1. Establish CPU, RSP, RDP and presentation measurements on a representative scene.
2. Add conservative object/chunk visibility and a stable geometry LOD policy.
3. Preprocess vertex batches and retain geometry command blocks.
4. Minimize texture uploads and material changes among visible objects.
5. Test resolution, Z, antialiasing, transparency and material complexity independently.
6. Specialize particles or other repeated workloads before replacing an entire renderer.

These priorities are engineering judgments developed in the following sections. They should be reordered when measurements identify a different critical path.

**Scope and evidence.** This reference emphasizes 3D engines, with 2D, particles and effects included. Source inspection covers current repository snapshots retrieved September 15, 2026. Published timing results are identified as author measurements; no new console benchmarks are claimed. Historical documentation and reconstructed commercial code are research references, while reusable implementations are identified separately.

<!-- pagebreak -->

## 1. Measure the limiting stage

Record frame time in milliseconds and processor work, not only an FPS counter. A nominal 60 Hz deadline is about 16.67 ms; 30 fps is about 33.33 ms. At 50 Hz, corresponding one- and two-refresh deadlines are 20 and 40 ms. Actual video timing and presentation policy must be recorded with the result.

Libdragon's RSPQ profiler exposes per-overlay slots, accumulated RDP busy ticks and frame counts. Its implementation names separate waits for CPU input, RDP output, full synchronization and overlay changes. These measurements help distinguish useful geometry work from time spent waiting.[^3][^4]

| Observed behavior | Working hypothesis | Discriminating experiment |
|---|---|---|
| RSP often waits for CPU | Submission or simulation starvation | Replay a retained scene; freeze simulation separately |
| Geometry overlay dominates | Transform, lighting or triangle setup limit | Reduce vertices; disable dynamic lights; simplify meshes |
| RSP waits for RDP; resolution reduction helps | Raster work or memory traffic limit | Reduce overdraw; simplify modes; reduce render area |
| Large spikes around material changes | Texture loads or state synchronization | Use one resident texture for an otherwise identical scene |
| CPU blocked awaiting a surface | Presentation pacing or insufficient buffering | Measure completion separately from framebuffer acquisition |

These are hypotheses, not one-to-one diagnoses. RDP busy time is not an isolated shader timer, and RSP time can include downstream stalls. Since work overlaps, adding all processor times does not reconstruct frame time.

Use the CPU profiler in `include/profile.h` for simulation, culling, animation and submission regions. RDPQ's validator and command logging help catch invalid mode combinations and unnecessary state churn. Disable logging and heavy debug overlays for final performance measurements.[^5][^7]

**Profiling build caveat.** Tiny3D's test-scene instructions require rebuilding libdragon with `RSPQ_PROFILE=1` and `RSPQ_DEBUG=0`, then rebuilding Tiny3D. The documented profiling configuration reduces available light slots and makes vertex effects unavailable/undefined because of DMEM pressure. It also adds RSP overhead even when application-level profiling is unused. Confirm those restrictions at the pinned version and repeat the winning configuration in a normal build.[^6]

**Benchmark protocol.** Use a deterministic camera path, identical assets, fixed random seeds, a warm-up interval, and several repeated runs. Capture median, 95th percentile and worst frame time; visible objects; submitted vertices/triangles; texture uploads; state changes; audio load; and wait time. This is a proposed experiment protocol, not a published N64 benchmark standard.

<!-- pagebreak -->

## 2. Visibility, scene structure and draw order

Cull before vertex submission and, where safe, before visual animation work. Nintendo's tuning guide describes grids, quadtrees/octrees, room connectivity, projected portal rectangles and hierarchical bounding volumes. The purpose is to avoid transforms, texture loads and other downstream processing for content that cannot contribute to the view.[^8]

**Choose spatial partitions that match the game.** Rooms and portals suit interiors. Terrain tiles or a quadtree suit large outdoor maps. An object BVH suits irregular collections. Keep gameplay and collision updates separate from visual visibility: an invisible actor may still require simulation.

Tiny3D's `examples/17_culling/main.c` is the most useful immediately transferable sample. It tests a whole-model bound, optionally queries a prebuilt BVH, and otherwise tests object bounds linearly. It records geometry per object while applying materials separately, preserving both culling granularity and cross-object state reuse. The example explicitly notes that a linear scan may beat a BVH at low object counts.[^9]

Two implementation details matter: bounds and frustum must use the same coordinate space, and the BVH visibility flags need resetting between queries. For moving or deforming geometry, conservative bounds must include the motion; stale bounds produce disappearing surfaces rather than a valid optimization.

### Balance visibility against batching

A world-sized material batch can minimize texture changes while forcing invisible geometry through the RSP. Extremely small chunks increase CPU tests, command calls and matrix changes. Benchmark several chunk sizes using the same camera path. Prefer independently visible chunks containing useful material runs.

For opaque geometry with depth testing, approximate front-to-back order can reduce depth-buffer writes. It does not make hidden triangles free: they still incur submission and geometry work, and the RDP depth test should not be treated as a modern hierarchical early-Z system. Nintendo explicitly recommends partial sorting rather than requiring an exact per-triangle sort.[^10]

**Proposed opaque sort key:** pass, coarse near-to-far depth bucket, material/texture group, mesh. Compare it against material-first sorting. Texture-heavy scenes may prefer material-first; heavy overlap may prefer depth-first. Blended transparency normally needs its own back-to-front ordering and typically disables depth writes while retaining depth tests.

### LOD and clipping

Reduce geometry using projected size and hysteresis, not distance alone. A wide building and a small prop need different thresholds. Distant billboards reduce transform and triangle work but can increase transparent pixel coverage. Nintendo's guide warns that replacing meshes with pictures is primarily a geometry optimization.[^11]

Use backface culling for appropriate closed meshes. Avoid needless geometry crossing the near plane; clipping creates extra work. Tiny3D's triangle path performs rejection and invokes separate clipping code when needed. Retain required clipping for correctness, and test large triangles at screen edges before changing guard-band behavior.[^16]

<!-- pagebreak -->

## 3. Geometry preprocessing and RSP efficiency

The exporter is part of the renderer. Convert assets into the runtime's packed vertex, matrix and command formats offline. Remove degenerate geometry and unused attributes, preserve material boundaries, and optimize the order in which triangles consume vertices. A generic interchange-format index buffer is not automatically a good N64 display stream.[^13][^48]

Tiny3D's inspected API defines a **70-entry transformed vertex cache**. Its mesh optimizer constructs cache-sized parts and combines contiguous vertex loads with triangle sequences. A small amount of vertex duplication can replace fragmented loads; optimize measured transfers and transforms rather than minimizing file size alone.[^13][^14]

### A published example with actual numbers

Tiny3D's optimization document reports the following for its test model. This is one author's scene and implementation at the time of writing, not a cross-engine benchmark or a guaranteed improvement for other assets.[^12]

| Metric | Before strips | With strips |
|---|---:|---:|
| Triangle-emission commands | 2,326 | 127 |
| Triangle/index command traffic | 18,608 bytes | 10,142 bytes |
| RSP time | 10,457 µs* | 8,807 µs |

*The earlier time is derived by adding the reported 1,650 µs saving to 8,807 µs. That is about 15.8% less RSP time, alongside about 45.5% less listed command/index traffic. Vertex count rose slightly. These figures do not include a claim of an equal whole-frame speedup.*

### Beyond ordinary triangle strips

The cache and temporary index storage share DMEM. Tiny3D can reclaim vertex slots once their final referencing triangles have been emitted. Its importer builds a schedule that respects these lifetimes. For a custom exporter, adapt the actual optimizer and format constraints rather than applying a desktop stripifier without a cache model.[^13]

The current API also has `t3d_tri_draw_unindexed()` and `t3d_quad_draw_unindexed()` for sequential layouts. These avoid index-buffer loading in suitable cases. This does not imply that duplicating all shared vertices is faster; use the path for naturally sequential primitives and compare total geometry cost.[^14]

### Microcode-level opportunities

Tiny3D's vertex code launches an asynchronous DMA, sets up matrices and other state, and processes packed vertex pairs. The triangle code separately handles rejection, coefficient generation and asynchronous output transfer. These are concrete examples of overlapping transfers with computation and keeping compact fast paths.[^15][^16]

For later specialization, consider fewer light modes, procedural repeated geometry, compressed input attributes or dedicated particles. Preserve DMA alignment, data lifetime, fixed-point range and overlay synchronization. A faster inner loop is useful only if it reduces the frame's limiting work; larger emitted RDP traffic can erase the gain.

<!-- pagebreak -->

## 4. Materials, TMEM and texture residency

The RDP has **4 KiB of TMEM and eight tile descriptors**. Descriptors are views of resident texture data, not eight independent 4 KiB allocations. Color-indexed textures use the lower half for indices and the upper half for palette lookup, so their index-data budget is 2 KiB.[^17]

The following are illustrative single-texture allocations computed from dimensions and bit depth. They assume valid row alignment and no competing textures or mip levels; they are not universal maximum dimensions.

| Format and dimensions | Texel payload | Practical implication |
|---|---:|---|
| RGBA16, 32 × 32 | 2,048 B | Leaves room for another compatible 2 KiB allocation |
| RGBA16, 64 × 32 | 4,096 B | Consumes the full nominal budget |
| RGBA32, 32 × 32 | 4,096 B | Expensive residency; format has bank-layout constraints |
| CI4, 64 × 64 | 2,048 B indices | Uses lower half; palette occupies upper-half storage |
| CI8, 64 × 32 | 2,048 B indices | Same lower-half limit with more colors |
| I4, 64 × 64 | 2,048 B | Useful for intensity masks and tintable effects |

**Optimize upload frequency first.** A smaller texture uploaded repeatedly can cost more than a slightly larger one reused across many objects. Build the visible draw list, group compatible surfaces, upload once, then draw the run. An atlas that sits in RDRAM still needs appropriate TMEM loading; its existence alone does not eliminate transfers.

Libdragon provides subtexture loading, multi-upload allocation and texture reuse helpers. `rdpq_tex_reuse()` and its subregion variant configure additional tile views without another upload, within the documented multi-upload context. Respect subregion alignment and actual TMEM availability; the inspected preview API also offers `rdpq_tex_can_upload()`.[^18]

Tiny3D's model layer tracks material state. Keep a `T3DModelState` across compatible objects so repeated texture hashes and settings can be recognized. If an unrelated drawing path changes RDP state, reestablish or invalidate the application's cached assumptions before continuing.[^19]

### Design materials for the fast path

Prefer baked vertex color or a texture-plus-vertex-color material when it meets the visual goal. Reserve multitexturing, lightmap passes, elaborate fog/blending and larger textures for visible benefits. Group render-mode updates with `rdpq_mode_begin()` / `rdpq_mode_end()`; inspect the resulting command stream because RDPQ selects one- or two-cycle operation from the requested state.[^20]

Mipmapping is valuable for image stability, but consumes texture storage and can require additional cycle work. Compare the actual mipmapped configuration against an appropriate lower-resolution texture. Tiny3D includes a working mipmap example; it should be treated as a quality/performance experiment rather than a universally faster path.[^46]

<!-- pagebreak -->

## 5. Pixel cost, depth and antialiasing

One-cycle and two-cycle operation have theoretical rates of one pixel per cycle and one pixel per two cycles. Real throughput is lower and scene-dependent because memory accesses, primitive setup, texture transfers and synchronization also matter. Disabling an effect only helps if it changes the work on the critical path.[^21]

### Start with a controlled framebuffer budget

At 320 × 240, one 16-bit color buffer is 153,600 bytes. Two such color buffers plus one 16-bit depth buffer total 460,800 bytes; three color buffers plus depth total 614,400 bytes. A 640 × 480 image has four times as many pixels. These arithmetic figures exclude other allocations and do not imply an exact fourfold rendering-time change.

Use 320 × 240 and 16-bit color as an initial speed-oriented experiment, then raise quality based on measurements. A larger display output does not require the same internal rendering dimensions. Libdragon's display configuration controls framebuffer dimensions, depth and buffering; record these settings in every comparison.[^22]

### Depth and overdraw

Keep depth testing for general overlapping 3D geometry. Draw opaque surfaces in approximate near-to-far order, limit large overlapping effects, and use conservative occlusion to prevent invisible work from reaching the RDP. Removing Z is an architectural choice requiring a valid visibility order, not a safe global toggle.

Nintendo describes depth-sorted convex objects, view-dependent terrain traversal, cells and layered scenes as opportunities to avoid Z. Cyclic overlap has no correct simple sorting order without subdivision or another visibility solution. A hybrid renderer can exploit sorted backgrounds while retaining depth for intersecting dynamic objects, but must explicitly handle their mutual occlusion.[^10]

### AA, filtering and coverage are different controls

Compare full RDP antialiasing, reduced antialiasing and no RDP AA. Reduced AA changes coverage handling and has visible limitations, especially where partially covered edges overlap. VI filtering is another stage; record it separately. Changing only the final display filter does not remove depth traffic or material work.[^10][^22]

Texture filtering and polygon AA are different features. Point sampling should be chosen for a measured cost or an intentional appearance, not assumed to halve rendering time. Ordinary one-cycle filtered materials are possible; two-cycle cost must be assessed from the complete material configuration.[^20][^21]

### Clears and synchronization

Use fill operations for rectangular clears. Omit a color clear only when every required pixel and coverage state is correctly established by later drawing; retain depth initialization unless an alternative scheme is designed. RDP pipe/tile/load synchronization protects state correctness. Atomic primitive mode adds inter-primitive delay; only change it with a demonstrated coherency case in mind.[^23]

RDPQ already manages many dependencies. Reduce redundant state changes and their required synchronization together. Blindly deleting synchronization commands can turn an apparent performance gain into intermittent rendering corruption.

<!-- pagebreak -->

## 6. Asynchronous scheduling and CPU memory

Retained geometry commands remove repeated CPU construction work. RSPQ supports recordable blocks and nested block execution, while RDPQ can store suitable commands directly in static RDP buffers. This helps both submission overhead and command transport. It does not cache the final transformed pixels: referenced vertices and matrices still participate in rendering.[^24][^25]

RDPQ uses RDRAM output buffers rather than XBUS transfers directly from RSP DMEM. Its implementation explains the tradeoff: tiny DMEM buffers can fill quickly and propagate RDP stalls back through the RSP to the CPU. Saving one transfer is not necessarily worth reducing pipeline overlap.[^25]

### Buffer ownership is a performance feature

Never modify a matrix, vertex array or texture still being read by queued work. Tiny3D's segmented-address example records a placeholder matrix reference and changes the segment binding for the current frame's buffer. Buffered skeleton and viewport helpers follow the same ownership idea.[^26]

For an engine with a custom scheduler, associate mutable resources with a completion token or a frame context that is known to be free. Merely taking `frame_number % 3` is insufficient if the scheduler can have more work outstanding than that allocation covers. Blocks containing references must outlive their pending execution.

Libdragon's attach API includes asynchronous completion callbacks and a presentation helper. Use these at meaningful boundaries instead of introducing a global wait after every object. Preserve required Tiny3D triangle/overlay synchronization when transitioning to other drawing paths; it is different from making the CPU wait for the entire frame.[^29][^14]

### Cache discipline

The CPU cache and DMA-visible memory are not automatically coherent. Write back cached CPU-produced data before a device reads it. For device-written data that the CPU will read, follow the invalidation and alignment rules; a dirty shared cache line can overwrite newer DMA results. Libdragon's `n64sys.h` documents these operations and cached/uncached address aliases.[^27]

Use cached memory for CPU-intensive data processing, and choose uncached allocation or targeted cache maintenance for shared rendering data. Do not make the entire engine uncached as a general solution. Keep hot draw records compact, reuse allocations and combine static transforms where possible; these are implementation recommendations to validate with CPU timings.

### Account for audio and presentation

The RSP also serves audio. Budget rendering with the final mixer workload active. Historical guidance explicitly schedules RDP work while the RSP handles another task, and overlaps CPU work with rendering.[^28]

Additional framebuffers can reduce idle time caused by missed presentation opportunities, but consume memory and can increase latency. They do not increase raster throughput. Measure framebuffer acquisition, RDP completion and display pacing independently before deciding whether double or triple buffering is appropriate.[^11]

<!-- pagebreak -->

## 7. Animation, particles, 2D and effects

### Animation and lighting

Precompute static transforms and baked lighting. For dynamic characters, measure animation evaluation separately from matrix submission and geometry. Tiny3D supports rigid/fake-blended skinning with one bone per vertex; this is not arbitrary multi-weight desktop skinning. Grouping a mesh coherently by its required matrix can reduce repeated bone switches.[^1][^48]

Apply visual animation LOD to distant actors and use conservative animated bounds. Lower evaluation frequency can save CPU work if interpolation and gameplay semantics permit it. Do not stop gameplay-critical animation events because an actor is offscreen. Measure directional and point-light paths separately rather than treating all lights as equal-cost.

### Specialized particles

TinyPX is a dedicated particle path within Tiny3D. It uses compact particle records and can submit a group with a single command, instead of requiring the application to construct ordinary geometry commands for every particle. It has separate state with helpers to inherit the Tiny3D camera/matrix setup.[^30]

Study `examples/18_particles/main.c` and `examples/19_particles_tex/main.c`. The latter demonstrates the textured path and texture-coordinate controls. It is a good starting point for repeated sparks, dust or sprite-like effects, subject to the API's size, precision and orientation limitations.[^31]

A geometry-efficient particle can still be raster-expensive. As an engineering policy, cap projected particle area and overlapping layers as well as particle count. Reduce the size or density of smoke near the camera, cull hidden emitters, and batch compatible textures without breaking necessary transparency order.

### 2D paths

Use RDP texture rectangles for compatible screen-aligned sprites. RDPQ COPY mode supports fast bitmap blitting but has a restricted contract: a 16-bit destination, supported texture formats, limited scaling, and no general blending or rotation. Standard mode is needed when those restrictions do not fit. The approximate fourfold draw-rate advantage documented for COPY is not an end-to-end sprite-engine guarantee.[^20]

Avoid repeated high-level blits that needlessly reload the same texture. For a resident sprite sheet, organize compatible draws into a batch. Use the actual RDPQ sample and upload APIs to check which layer controls residency and which call emits another load.[^18][^47]

### Expensive effects with concrete alternatives

James Lambert's graphics demos show a low-resolution projected shadow image and a separate multipass shadow-volume technique. The latter redraws receivers and volumes several times; it demonstrates an effect rather than a free optimization.[^32]

Prefer a blob/projected shadow for small actors when acceptable. Restrict mirrors and portal views to their visible area and update rate. Benchmark bloom, outlines and large-texture pipelines separately; a visually impressive demo proves feasibility, not that its technique makes ordinary opaque geometry faster.

<!-- pagebreak -->

## 8. What earlier engines teach us

### Perfect Dark: rooms and portals

The reconstructed `src/game/bg.c` maintains room/portal data, determines visible room regions, and applies scissor rectangles in its background rendering paths. It is a concrete way to study how visibility data, draw order and screen bounds interact in an N64 engine. Treat reconstruction names and comments as the decompilation project's interpretation, not original developer documentation.[^34]

**Transferable lesson:** build visibility into the scene format. A portal system needs authored connectivity and conservative traversal; adding a generic per-object frustum test later cannot reproduce its occlusion benefits. Do not copy commercial game code or assets into a new independently licensed runtime.

### World Driver Championship / Stunt Racer: ZSort

GLideN64's authors report that BOSS used a substantially modified ZSort microcode, with graphics and audio fitting into a compact implementation. Their analysis describes far-to-near sorting to avoid depth-buffer work. This is evidence of game-specific visibility and scheduling design; its reported fill-rate benefit is not a universal speed multiplier.[^35]

**Transferable lesson:** constrained content can trade sorting and authoring restrictions for less per-pixel memory traffic. A fixed-camera or layered renderer has more opportunity than an unrestricted world with intersecting objects.

### Factor 5: specialized microcode commands

The developers who reverse-engineered the Indiana Jones and Battle for Naboo microcodes describe specialized lighting, particles and terrain functionality. Their host-side interpretation includes separate particle generation and terrain polygon paths. This is original reverse-engineering evidence, not a release of Factor 5's reusable engine source.[^36][^37]

**Transferable lesson:** change the submitted representation when a workload repeats. One command describing a particle batch or terrain structure can be more effective than accelerating thousands of generic CPU calls. TinyPX is a modern independent example of that direction.

### F3DEX3: useful ideas with a provenance boundary

F3DEX3 documents a 56-vertex cache, generalized triangle snakes, suppression of redundant texture loads, an occlusion-plane system and detailed performance counters. It also distinguishes cheaper directional lighting from more expensive point lighting. These are useful features to investigate in a renderer whose bottleneck is known.[^38]

The project says its modifications and CPU code are dedicated to the public domain, but the microcode is a modification of F3DEX2. That statement does not turn the entire Nintendo-derived base into an independently licensed SDK. Use Tiny3D as the default implementation source for an independent runtime; treat F3DEX3 as a research and ROM-hacking reference unless the required rights are independently established.[^38]

Historical engines succeeded through combinations of content constraints, visibility, command representation, memory management and material choices. No single polygon-per-second number captures those tradeoffs.

<!-- pagebreak -->

## 9. Source-code map: start here

All Tiny3D and libdragon paths below were checked against the snapshots listed in section 10. Citation links open exact files where available. Example assets can have licenses distinct from the code.

| Priority | Repository and path | What to inspect |
|---|---|---|
| 1 | Tiny3D: `examples/99_testscene/Readme.md` | Profiling setup and build restrictions [^6] |
| 1 | libdragon: `include/rspq_profile.h`; `src/rspq/rspq_profile.c` | Counters, wait categories and overlay time [^3][^4] |
| 1 | libdragon: `include/profile.h` | CPU regions, frame accounting and reports [^5] |
| 1 | Tiny3D: `examples/17_culling/main.c` | Object blocks, BVH queries, material state [^9] |
| 1 | Tiny3D: `examples/03_objects/main.c` | Shared model blocks and per-object matrices [^45] |
| 1 | Tiny3D: `examples/11_segments/main.c` | Buffered matrices, skeletons, segment rebinding [^26] |

**First reading pass:** understand how the test scene measures work, how the culling example retains independently visible objects, and how segment binding selects mutable data for the current frame. These address common engine-level costs before low-level microcode changes.

The current source tree's basic quad example is `examples/00_quad/main.c`; the retrieved top-level README still names `01_quad`. Prefer the checked tree over that stale path.

<!-- pagebreak -->

## 9. Source-code map: exporter and microcode

These files explain how geometry and material choices become transfers and commands. Start with the high-level implementation; descend into microcode only when the profiler shows that its work is limiting the frame.

| Priority | Repository and path | What to inspect |
|---|---|---|
| 2 | Tiny3D: `tools/gltf_importer/src/optimizer/meshOptimizer.cpp` | Cache parts, contiguous loads and index scheduling [^13] |
| 2 | Tiny3D: `docs/modelOpt.md`; `docs/modelFormat.md` | Design rationale, measured example, binary layout [^12][^48] |
| 2 | Tiny3D: `src/t3d/t3dmodel.c` | Material state tracking and model draw commands [^19] |
| 2 | libdragon: `include/rdpq_tex.h`; `include/rdpq_mode.h` | Upload residency and material-mode constraints [^18][^20] |
| 2 | Tiny3D: `src/t3d/tpx.h`; `examples/19_particles_tex/main.c` | Dedicated particle submission and texture behavior [^30][^31] |
| 2 | libdragon: `examples/rdpqdemo/rdpqdemo.c` | Small working RDPQ drawing example [^47] |
| 3 | Tiny3D: `src/t3d/rsp/rsp_tiny3d.rspl` | Vertex loads, transforms, lights and DMA overlap [^15] |
| 3 | Tiny3D: `src/t3d/rsp/rspq_triangle.rspl` | Rejection, clipping dispatch, setup and output [^16] |

**Second reading pass:** follow one converted mesh through the format description, optimizer, model draw function and triangle path. Record its vertex-load count, triangle commands and texture loads. This makes a later optimization concrete enough to test.

For a custom engine, the most useful unit to retain is generally independently visible geometry. A single retained block for an entire world is easy to replay but difficult to cull, sort or stream effectively. This is a proposed architecture informed by the object-culling sample, not an API requirement.

<!-- pagebreak -->

## 10. Alternative stacks, licenses and version pins

| Resource | Role in a performance project | Reuse position |
|---|---|---|
| libdragon | SDK, RDPQ, profiling, DMA, display and audio | Top-level Unlicense/public-domain dedication; review separately licensed components [^42] |
| Tiny3D | Native 3D pipeline and exporter | MIT; project states microcode was written independently [^1][^43] |
| Pyrite64 | Complete engine/editor and integration example | MIT; vendored components have separate licenses [^39] |
| libdragon OpenGL | Porting an existing fixed-function renderer | Inspect compiled display lists and unsupported RSP features [^33] |
| F3DEX3 | Advanced microcode and profiling ideas | Nintendo-derived base; not a blanket independent-SDK recommendation [^38] |
| Commercial decompilations | Historical architecture research | Public availability does not establish permission to reuse game code/assets |
| Portal 64: Still Alive | Portal-renderer research lead | Its retrieved dependency document still lists proprietary components [^44] |

Tiny3D's example culling scene includes asset credits such as CC BY 4.0 and CC0; the code's MIT license does not replace those asset terms. Keep asset provenance with exported content. No proprietary SDK archive, commercial ROM, or extracted game asset is supplied with this report.[^9]

The reuse positions above describe the inspected project declarations and dependency notes. A top-level license should not be applied to unrelated third-party code or sample assets without checking their own terms.

<!-- pagebreak -->

## 10. Integration alternatives and version pins

### Pyrite64 integration references

Inspect `n64/engine/src/renderer/drawLayer.cpp` for separate geometry, particle and 2D layers with buffered queues. Inspect `n64/engine/src/renderer/pipelineDefault.cpp` for framebuffer ownership, completion callbacks and configurable clears. Its big-texture/HDR pipelines are optional advanced examples, not the default recommendation for fastest conventional rendering.[^40][^41]

### If an OpenGL renderer already exists

Libdragon's OpenGL guide explains why compiled display lists can avoid repeated fixed-point conversion and vertex-cache bookkeeping. It also notes that VBOs are RAM buffers on this UMA system, and that some features can trigger CPU fallback. The guide dates from 2024; verify the current implementation before assuming its full fallback list still applies. Measure the optimized existing path before paying for a rewrite.[^33]

### Reproducible source snapshots

| Repository / branch inspected | Commit |
|---|---|
| `HailToDodongo/tiny3d` / `main` | `73d822f3af7c85cc1b93eaa4546504ba0a4ea156` |
| `DragonMinded/libdragon` / `preview` | `39d0d6096130836a65710da6730290701d060272` |
| `HailToDodongo/pyrite64` / `main` | `ce7515644001d1366cad04280b3a73d4b393cb42` |

These identify inspected source, not a tested compatible release set. Tiny3D's README requires libdragon's preview branch. Pin the toolchain and asset converter too, then build the selected examples together before integration. Historical and unpinned research links remain explicitly distinct from these snapshots.

<!-- pagebreak -->

## 11. Implementation patterns

The following are original schematic examples based on the cited APIs. They omit initialization and project-specific types and have not been compiled or tested on hardware. Use the linked upstream examples for complete programs.

### Record geometry once, apply visible material state each frame

```c
// Asset setup: one block per independently visible object.
for_each_object(model, object) {       // engine helper
    rspq_block_begin();
    t3d_model_draw_object(object, NULL);
    object->userBlock = rspq_block_end();
}

// Frame: visible[] has already been culled and sorted.
T3DModelState state = t3d_model_state_create();
for_each_visible(visible, draw) {      // engine helper
    t3d_matrix_set(draw->frame_matrix, true);
    t3d_model_draw_material(draw->object->material, &state);
    rspq_block_run(draw->object->userBlock);
}
t3d_tri_sync(); // Required when switching drawing overlays.
```

The culling example provides the object/material separation; the multiple-object example supplies the matrix-plus-block pattern. Matrices and referenced geometry must remain valid through queued consumption. Reset cached state after external changes that invalidate it.[^9][^45]

### Profile across repeated frames

```c
// With a compatible profiling-enabled SDK/microcode build:
rspq_profile_start();

// Mark equivalent points in each iteration of the frame loop.
rspq_profile_next_frame();

// Read accumulated data periodically, outside hot drawing code.
rspq_profile_data_t sample = {0};
rspq_profile_get_data(&sample);
// Compare slot times, RDP busy ticks, and frame count.
```

The API is asynchronous; freshly marked work need not already be accumulated. Maintain a consistent reporting interval and observe the profiler's build restrictions. CPU region instrumentation is a separate concern.[^3][^4][^6]

### Proposed exporter outputs

Emit spatial chunks; object bounds; LOD variants; packed geometry; per-chunk material runs; optimized cache parts; and stable asset identifiers. Store optional room/portal or BVH data alongside the level. Track emitted bytes, duplicated vertices and draw commands in the build report.

At runtime, acquire a completed frame context, determine visibility, choose LODs, prepare matrices, emit opaque and transparent passes, then particles/UI. Return ownership through a completion event. This engine outline is a recommendation, not a drop-in implementation supplied by either SDK.

<!-- pagebreak -->

## 12. Experiments and implementation priorities

Begin with a small representative benchmark scene plus a worst-case camera path from the intended game. Keep an untouched baseline. Implement one change per comparison and record image differences as well as timing. The matrix below is an actionable experiment plan; its outcomes are not predetermined.

| Order | Experiment | Measurements and decision |
|---|---|---|
| 1 | Replay retained geometry with simulation frozen | Separates CPU preparation from queued rendering |
| 2 | Compare linear bounds, BVH and room/portal visibility | CPU culling time versus work actually avoided |
| 3 | Compare chunk sizes and geometry LOD thresholds | Visible vertices, commands, clipping and pop-in |
| 4 | Compare material-first and coarse depth-first ordering | Uploads, state changes, RDP time and CPU sort cost |
| 5 | Substitute one small resident texture | Tests texture/state overhead without changing geometry |
| 6 | Reduce internal dimensions while preserving scene/FOV | Tests sensitivity to pixel work; record clipping changes |
| 7 | Compare full/reduced/no AA and simpler materials | RDP time and visible quality tradeoffs |
| 8 | Reduce particle area/layers; try TinyPX | Distinguishes submission savings from fill cost |
| 9 | Restore final animation and audio workloads | Checks that rendering gains survive the complete game |
| 10 | Rebuild without profiling and retest on console | Confirms production performance and timing correctness |

### When to consider larger architectural changes

Choose a sorted, partially Z-free renderer only if content and camera rules give a correct ordering. Consider a dedicated terrain or particle microcode when those workloads dominate measured RSP time. Consider a lower internal resolution when the RDP is limiting and the quality tradeoff is acceptable. Preserve an optimized OpenGL path if it meets the target and a replacement would add unnecessary complexity.

### Common misleading optimizations

Fewer source triangles do not guarantee fewer transformed vertices or commands. A larger atlas does not guarantee fewer TMEM uploads. Hiding distant geometry in fog does not itself stop its rendering. A billboard saves geometry but can increase overdraw. Removing a CPU wait can expose a data race. Triple buffering improves scheduling opportunities rather than intrinsic pixel throughput.

### Immediate engineering sequence

**First milestone:** a reproducible baseline with CPU regions, RSP wait categories, RDP busy measurements and a stable camera replay.

**Second milestone:** per-object/chunk retained geometry, correct frame-buffered mutable data, conservative visibility and a measured sort policy.

**Third milestone:** offline mesh optimization, texture-residency accounting and deliberate material quality tiers. Require a measurable improvement before adding specialized microcode.

**Acceptance criteria:** improved representative and worst-case frame times, no new visual errors along the test path, no ownership/cache hazards, and correct output on real hardware. Accurate low-level emulation is useful for development, but host emulator FPS is not console performance evidence.

## Sources

[^1]: Max Bebök / HailToDodongo. [Tiny3D project overview](https://github.com/HailToDodongo/tiny3d). Undated; accessed September 15, 2026.

[^2]: libdragon contributors. [RDPQ hardware-accelerated drawing API](https://libdragon.dev/ref/group__rdpq.html). Undated; accessed September 15, 2026.

[^3]: libdragon contributors. [RSPQ profiling API: include/rspq_profile.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rspq_profile.h). Undated; accessed September 15, 2026.

[^4]: libdragon contributors. [RSPQ profiling implementation: src/rspq/rspq_profile.c](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/src/rspq/rspq_profile.c). Undated; accessed September 15, 2026.

[^5]: libdragon contributors. [CPU profiling API: include/profile.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/profile.h). Undated; accessed September 15, 2026.

[^6]: Max Bebök / HailToDodongo. [Performance test scene and profiling restrictions](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/examples/99_testscene/Readme.md). Undated; accessed September 15, 2026.

[^7]: libdragon contributors. [RDPQ debugging API](https://libdragon.dev/ref/rdpq__debug_8h.html). Undated; accessed September 15, 2026.

[^8]: Nintendo of America; archived by ultra64.ca. [Game World Organization, Programming Manual §24.1](https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro24/24-01.html). January 1999.

[^9]: Max Bebök / HailToDodongo. [BVH culling example: examples/17_culling/main.c](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/examples/17_culling/main.c). Undated; accessed September 15, 2026.

[^10]: Nintendo of America; archived by ultra64.ca. [Raster Tuning, Programming Manual §24.4](https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro24/24-04.html). January 1999.

[^11]: Nintendo; archived by ultra64.ca. [Tuning Performance, Introductory Manual §4.2](https://ultra64.ca/files/documentation/online-manuals/man-v5-1/kantan/step2/4/4_2.htm). Legacy manual; accessed September 15, 2026.

[^12]: Max Bebök / HailToDodongo. [Model Pre-Processing / Optimizations](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/docs/modelOpt.md). Undated; accessed September 15, 2026.

[^13]: Max Bebök / HailToDodongo. [Mesh optimizer implementation](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/tools/gltf_importer/src/optimizer/meshOptimizer.cpp). Undated; accessed September 15, 2026.

[^14]: Max Bebök / HailToDodongo. [Tiny3D API: src/t3d/t3d.h](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/src/t3d/t3d.h). Undated; accessed September 15, 2026.

[^15]: Max Bebök / HailToDodongo. [Vertex microcode: src/t3d/rsp/rsp_tiny3d.rspl](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/src/t3d/rsp/rsp_tiny3d.rspl). Undated; accessed September 15, 2026.

[^16]: Tiny3D contributors. [Triangle microcode: src/t3d/rsp/rspq_triangle.rspl](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/src/t3d/rsp/rspq_triangle.rspl). Undated; accessed September 15, 2026.

[^17]: Nintendo of America; archived by ultra64.ca. [Texture Engine, Programming Manual §12.4](https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro12/12-04.html). January 1999.

[^18]: libdragon contributors. [Texture upload and reuse API: include/rdpq_tex.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_tex.h). Undated; accessed September 15, 2026.

[^19]: Tiny3D contributors. [Model/material drawing: src/t3d/t3dmodel.c](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/src/t3d/t3dmodel.c). Undated; accessed September 15, 2026.

[^20]: libdragon contributors. [Render mode API: include/rdpq_mode.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/rdpq_mode.h). Undated; accessed September 15, 2026.

[^21]: Paris Oplopoios. [The RDP as I understand it](https://offtkp.github.io/RDP/). 2023; accessed September 15, 2026.

[^22]: libdragon contributors. [Display buffers: include/display.h](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/display.h). Undated; accessed September 15, 2026.

[^23]: Nintendo of America; archived by ultra64.ca. [RDP Global State, Programming Manual §12.2](https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro12/12-02.html). January 1999.

[^24]: libdragon contributors. [RSP command queue API](https://libdragon.dev/ref/rspq_8h.html). Undated; accessed September 15, 2026.

[^25]: libdragon contributors. [RDPQ implementation architecture](https://libdragon.dev/ref/rdpq_8c.html). Undated; accessed September 15, 2026.

[^26]: Max Bebök / HailToDodongo. [Buffered matrices and segmented addresses example](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/examples/11_segments/main.c). Undated; accessed September 15, 2026.

[^27]: libdragon contributors. [N64 system and cache interfaces](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/include/n64sys.h). Undated; accessed September 15, 2026.

[^28]: Nintendo of America; archived by ultra64.ca. [CPU Tuning, Programming Manual §24.5](https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro24/24-05.html). January 1999.

[^29]: libdragon contributors. [RDP attachment and completion API](https://libdragon.dev/ref/rdpq__attach_8h.html). Undated; accessed September 15, 2026.

[^30]: Max Bebök / HailToDodongo. [TinyPX particle API: src/t3d/tpx.h](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/src/t3d/tpx.h). Undated; accessed September 15, 2026.

[^31]: Max Bebök / HailToDodongo. [Textured particles example](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/examples/19_particles_tex/main.c). Undated; accessed September 15, 2026.

[^32]: James Lambert. [N64 graphics demos: shadows and palette effects](https://github.com/lambertjamesd/n64graphicsdemo). Undated; accessed September 15, 2026.

[^33]: Giovanni Bajo / libdragon. [OpenGL on N64](https://github.com/DragonMinded/libdragon/wiki/OpenGL-on-N64). Edited October 1, 2024; accessed September 15, 2026.

[^34]: Perfect Dark decompilation contributors. [Perfect Dark background and portal renderer reconstruction](https://github.com/n64decomp/perfect_dark/blob/master/src/game/bg.c). Undated; accessed September 15, 2026.

[^35]: Gonetz / GLideN64. [HLE implementation of BOSS ZSort microcode](https://gliden64.blogspot.com/2019/02/hle-implementation-of-boss-zsort.html). February 18, 2019.

[^36]: Gonetz / GLideN64. [HLE implementation of Indiana Jones and Battle for Naboo microcodes](https://gliden64.blogspot.com/2018/05/hle-implementation-of-microcodes-for.html). May 26, 2018.

[^37]: GLideN64 contributors. [Factor 5 microcode interpretation: F5Indi_Naboo.cpp](https://github.com/gonetz/GLideN64/blob/master/src/uCodes/F5Indi_Naboo.cpp). Undated; accessed September 15, 2026.

[^38]: Sauraen / HackerN64 contributors. [F3DEX3 features, performance improvements, and provenance](https://github.com/HackerN64/F3DEX3). Undated; accessed September 15, 2026.

[^39]: Max Bebök / HailToDodongo. [Pyrite64 engine overview and licensing](https://github.com/HailToDodongo/pyrite64). Undated; accessed September 15, 2026.

[^40]: Max Bebök / HailToDodongo. [Pyrite64 draw-layer implementation](https://github.com/HailToDodongo/pyrite64/blob/ce7515644001d1366cad04280b3a73d4b393cb42/n64/engine/src/renderer/drawLayer.cpp). Undated; accessed September 15, 2026.

[^41]: Max Bebök / HailToDodongo. [Pyrite64 default render pipeline](https://github.com/HailToDodongo/pyrite64/blob/ce7515644001d1366cad04280b3a73d4b393cb42/n64/engine/src/renderer/pipelineDefault.cpp). Undated; accessed September 15, 2026.

[^42]: libdragon contributors. [Libdragon license](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/LICENSE.md). Undated; accessed September 15, 2026.

[^43]: Max Bebök / HailToDodongo. [Tiny3D license](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/LICENSE). Undated; accessed September 15, 2026.

[^44]: Portal 64: Still Alive contributors. [Portal 64: Still Alive - N64 Library Usage](https://github.com/mwpenny/portal64-still-alive/blob/master/documentation/n64_library_usage.md). Undated; accessed September 15, 2026.

[^45]: Max Bebök / HailToDodongo. [Multiple-object rendering example](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/examples/03_objects/main.c). Undated; accessed September 15, 2026.

[^46]: Max Bebök / HailToDodongo. [Mipmap example](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/examples/20_mipmaps/main.c). Undated; accessed September 15, 2026.

[^47]: libdragon contributors. [RDPQ demo: examples/rdpqdemo/rdpqdemo.c](https://github.com/DragonMinded/libdragon/blob/39d0d6096130836a65710da6730290701d060272/examples/rdpqdemo/rdpqdemo.c). Undated; accessed September 15, 2026.

[^48]: Max Bebök / HailToDodongo. [Tiny3D model file format](https://github.com/HailToDodongo/tiny3d/blob/73d822f3af7c85cc1b93eaa4546504ba0a4ea156/docs/modelFormat.md). Undated; accessed September 15, 2026.
