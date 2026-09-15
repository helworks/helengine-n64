#include "platform/n64/Nintendo64RuntimeMaterial.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

namespace helengine::n64 {

Nintendo64RuntimeMaterial::Nintendo64RuntimeMaterial(uint32_t color, bool doubleSided)
    : ::RuntimeMaterial(), ColorValue(color), DoubleSidedValue(doubleSided) {
    LightingModel = RuntimeMaterialLightingModel::Unlit;
    SupportsNormalMapping = false;
    SupportsEmissive = false;
    CastsShadows = false;
    ReceivesShadows = false;
    set_Id("n64-unlit");
}

} // namespace helengine::n64

#endif
