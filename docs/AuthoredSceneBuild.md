# Authored N64 scene build

The authored mode is selected with `-Mode authored` and requires a generated
Core tree. It refuses to fall back to the diagnostic cube when `Core.hpp` is
missing. Diagnostic mode remains available with `-Mode diagnostic`.

The generic editor/build graph supplies the generated Core and cooked manifest.
The N64 builder then stages only manifest-declared scene/material/model payloads,
writes `n64_runtime_scene_catalog.hpp/.cpp`, and invokes the existing Docker
toolchain. The runtime initializes DragonFS, loads the startup scene through
`Core::SceneManager`, and sends its camera and drawable submissions through the
Tiny3D renderer.

The current runtime supports one perspective camera, one static triangle-list
mesh, and one opaque unlit material. Authored world position, scale, rotation,
camera transform/projection, and base color are read from the generated runtime
objects. The old hardcoded cube is only used in diagnostic mode.

Build outputs and generated sources for the v0.2 checkpoint are kept under
`C:\dev\helworks\builds\helengine-n64\v02`.
