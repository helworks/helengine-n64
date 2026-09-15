# N64 generated runtime compatibility

This document records the generated Core and native ABI checkpoint for the N64
authored-scene path.

## Published contract

- Platform: `n64`
- Graphics profile: `n64-unlit`
- Material schema: `n64-unlit-v1`
- Build profiles: `debug`, `release`
- Serialization: generated C++ profile declares `LittleEndian`; cooked payloads
  are decoded through `AssetSerializer` and generated serializers on the target.
- Runtime material/model resolution: cooked platform-owned payloads.
- Runtime storage: manifest-declared files staged into DragonFS and exposed as
  content-relative `rom:/` paths.
- Unsupported features: textures, lighting, transparency, audio, physics,
  input, UI, animation, and lights are rejected by metadata or cooking.

## Generated ABI record

The generated Core is produced from `engine/helengine.core/helengine.core.csproj`
with the pinned C++ generator and the N64 profile:

```text
--platform n64 --language cpp --endianness little
--preset native-core-boot
--set generated-math-convention=native-column-vector
--set pointer-size-bytes=4
--set native-file-system-header="platform/n64/Nintendo64ContentStreamSource.hpp"
--set native-file-system-type=helengine::n64::Nintendo64ContentStreamSource
```

The authored build uses the generic generated interfaces for:

- `Core::Initialize`, `Core::Update`, and `Core::Draw`
- `SceneManager::LoadScene` and frame-boundary commit
- `RenderManager3D::BuildModelFromCooked`,
  `BuildMaterialFromCooked`, `Draw`, and release hooks
- `IContentStreamSource::OpenRead` and `AssetSerializer::Deserialize`
- `ObjectManager` camera and drawable lists

The N64 Makefile links the generator's final `helengine_core_unity.cpp` so
template definitions such as `EngineBinaryReader::ReadArray` and
`ContentManager::Load` are present with their generated call sites. This final
unit also includes the runtime component deserializers discovered from the
cooked scenes. The earlier `generated_unity.cpp` is not a valid authored-build
input because it can contain an empty component registration. The builder-owned
`n64_runtime_scene_catalog.cpp` is linked as a separate source; its startup
hooks use C linkage.

## Target constraints

- 320x240, 16-bit color, double buffering, and a 16-bit depth surface.
- Explicit `NO_LIGHT`, depth compare/write, `AA_NONE`, and back-face culling
  for the default opaque material.
- One perspective camera and one 3D drawable are required at runtime.
- Authored camera and drawable entities use layer mask `1`; scene loading
  preserves the serialized mask before camera queue filtering.
- One triangle-list submesh is supported, with at most 64 packed vertices and
  128 triangles. Non-finite coordinates, out-of-range fixed-point positions,
  invalid indices, and excess geometry fail the load.
- DragonFS is initialized with `DFS_DEFAULT_LOCATION`; the content bridge
  reads and closes each file before returning a memory-backed stream.

The authored cross-build links successfully with the pinned
`helengine-n64-toolchain` image. The resulting authored ELF is below the base
4 MiB RDRAM target's static image size, but target execution and heap headroom
remain unverified without an emulator or hardware capture.
