#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include "platform/n64/Nintendo64FrameLights.hpp"

class ObjectManager;

namespace helengine::n64 {

/// Fills Nintendo64FrameLights from the generated ObjectManager light lists.
class Nintendo64LightExtractor {
public:
    /// Sums every registered ambient light into the frame ambient color and converts
    /// every registered directional light into a Tiny3D color and surface-to-light vector.
    /// Throws InvalidOperationException when more than seven directional lights are
    /// registered, a light color is not finite, or a light direction is degenerate.
    static void Extract(::ObjectManager* objectManager, Nintendo64FrameLights& lights);
};

} // namespace helengine::n64

#endif
