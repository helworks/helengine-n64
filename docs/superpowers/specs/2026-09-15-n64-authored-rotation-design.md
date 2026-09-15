# N64: run the existing cube_test scene

Status: revised to follow Helena's direction to reuse cube_test.

## Outcome

Build and boot the existing demodisc scene `C:\dev\helprojs\demodisc\assets\scenes\rendering\cube_test.helen` on N64. Use its existing `city.rendering.CubeTestSpinComponent`, authored properties, mesh, and camera. Do not create another rotation component or duplicate the scene in the N64 fixture generator.

## Confirmed source

`assets/codebase/rendering/CubeTestSpinComponent.cs` derives from UpdateComponent, evaluates yaw from Core.Instance.TotalElapsedSeconds, normalizes its quaternion, and writes Parent.LocalOrientation. CubeTestSceneFactory attaches it with an angular speed of pi/2 radians per second (90 degrees per second).

The current factory also includes orbit-camera controls, a directional light, UI, instruction overlays, and a blueprint. The serialized scene and the platform's existing cooking/profile rules must be inspected before assuming all of these run on the minimal N64 backend.

## Implementation plan

1. Inspect the actual existing scene and its dependency/component inventory through engine serialization. Identify which dependencies the N64 profile already supports and report concrete blockers.
2. Point the N64 build/startup selection at cube_test through the normal project build path. Generate gameplay C++ and runtime deserializers from the existing demodisc source assembly. Preserve authored component identities and settings.
3. Fix only evidenced N64 integration gaps. Verify quaternion-to-Tiny3D matrix conversion for nonidentity rotations. Do not rewrite generated C++, animate from the native host, silently remove unsupported scene components, or change the shared demodisc scene merely to pass this test.
4. Build with the pinned toolchain and run focused tests for changed integration points. Validate scene loading, update lifecycle, and visible cube rotation in Gopher64; record final ROM path/hash and evidence.

If the actual scene requires broader UI/input/lighting support than this milestone can provide, identify the exact unsupported components and propose the smallest explicit platform-profile solution before expanding implementation.

## Execution

Parent plans and validates; one Luna xhigh agent implements. Use the existing helengine-n64-impl checkout and visible outputs under C:\dev\helworks\builds\helengine-n64. Preserve unrelated changes and use local commits without pushing or merging.
