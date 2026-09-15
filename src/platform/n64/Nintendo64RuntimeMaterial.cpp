#include "platform/n64/Nintendo64RuntimeMaterial.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

namespace helengine::n64 {

/// Creates one material from cooked color, culling, and lit flags.
Nintendo64RuntimeMaterial::Nintendo64RuntimeMaterial(uint32_t color, bool doubleSided, bool lit)
    : ::RuntimeMaterial(), ColorValue(color), DoubleSidedValue(doubleSided), LitValue(lit) {
    LightingModel = lit ? RuntimeMaterialLightingModel::MetalRoughPbr : RuntimeMaterialLightingModel::Unlit;
    SupportsNormalMapping = false;
    SupportsEmissive = false;
    CastsShadows = false;
    ReceivesShadows = false;
    set_Id("n64-standard");
}

} // namespace helengine::n64

#endif
