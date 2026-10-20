#include "platform/n64/Nintendo64RuntimeMaterial.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include "platform/n64/Nintendo64RenderManager2D.hpp"
#include "platform/n64/Nintendo64RuntimeTexture.hpp"
#include "runtime/native_exceptions.hpp"

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

Nintendo64RuntimeMaterial::~Nintendo64RuntimeMaterial() { Dispose(); }

void Nintendo64RuntimeMaterial::SetTexture(Nintendo64RenderManager2D& owner, Nintendo64RuntimeTexture* texture) {
    if (DisposedValue || texture == nullptr || texture->Surface() == nullptr) {
        throw new InvalidOperationException("N64 material cannot own a null, disposed or uninitialized texture.");
    }
    if (TextureValue == texture) {
        if (TextureOwner != &owner) throw new InvalidOperationException("N64 material texture has a different allocation owner.");
        return;
    }
    if (TextureValue != nullptr) TextureOwner->ReleaseTexture(TextureValue);
    TextureOwner = &owner;
    TextureValue = texture;
    SetPrimaryTexture(texture);
}

void Nintendo64RuntimeMaterial::Dispose() {
    if (DisposedValue) return;
    DisposedValue = true;
    Nintendo64RuntimeTexture* texture = TextureValue;
    TextureValue = nullptr;
    SetPrimaryTexture(nullptr);
    if (texture != nullptr) TextureOwner->ReleaseTexture(texture);
    TextureOwner = nullptr;
    ::RuntimeMaterial::Dispose();
}

} // namespace helengine::n64

#endif
