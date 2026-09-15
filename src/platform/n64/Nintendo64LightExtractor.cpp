#include "platform/n64/Nintendo64LightExtractor.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include "AmbientLightComponent.hpp"
#include "DirectionalLightComponent.hpp"
#include "LightDirectionUtility.hpp"
#include "ObjectManager.hpp"
#include "float3.hpp"
#include "float4.hpp"
#include "runtime/native_exceptions.hpp"
#include "runtime/native_list.hpp"

#include "platform/n64/Nintendo64LightMath.hpp"

namespace helengine::n64 {

void Nintendo64LightExtractor::Extract(::ObjectManager* objectManager, Nintendo64FrameLights& lights) {
    if (objectManager == nullptr) {
        throw new ArgumentNullException("objectManager");
    }

    float ambientAccumulator[3] = {0.0f, 0.0f, 0.0f};
    ::List<::AmbientLightComponent*>* ambientLights = objectManager->get_AmbientLights();
    if (ambientLights != nullptr) {
        for (int32_t index = 0; index < ambientLights->get_Count(); index++) {
            ::AmbientLightComponent* ambient = (*ambientLights)[index];
            if (ambient == nullptr) {
                continue;
            }
            const ::float4 color = ambient->get_Color();
            Nintendo64LightMath::Accumulate(color.X, color.Y, color.Z, ambient->get_Intensity(), ambientAccumulator);
        }
    }
    Nintendo64LightMath::PackAccumulated(ambientAccumulator, lights.Ambient);

    lights.DirectionalCount = 0;
    ::List<::DirectionalLightComponent*>* directionalLights = objectManager->get_DirectionalLights();
    if (directionalLights == nullptr) {
        return;
    }
    if (directionalLights->get_Count() > kMaxDirectionalLights) {
        throw new InvalidOperationException("N64 lighting supports at most seven directional lights.");
    }
    for (int32_t index = 0; index < directionalLights->get_Count(); index++) {
        ::DirectionalLightComponent* directional = (*directionalLights)[index];
        if (directional == nullptr) {
            continue;
        }
        Nintendo64DirectionalLight& entry = lights.Directional[lights.DirectionalCount];
        const ::float4 color = directional->get_Color();
        Nintendo64LightMath::PackColor(color.X, color.Y, color.Z, directional->get_Intensity(), entry.Color);
        const ::float3 forward = ::LightDirectionUtility::GetLightDirection(directional);
        if (!Nintendo64LightMath::ToLightVector(forward.X, forward.Y, forward.Z, entry.Direction)) {
            throw new InvalidOperationException("N64 directional light has a zero-length direction.");
        }
        lights.DirectionalCount++;
    }
}

} // namespace helengine::n64

#endif
