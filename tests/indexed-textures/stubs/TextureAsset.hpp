#pragma once
#include <cstdint>
#include "runtime/array.hpp"
enum class TextureAssetColorFormat { Rgba32, Rgba4444, Indexed4, Indexed8 };
enum class TextureAssetAlphaPrecision { Opaque, Binary, A4, A8 };
struct TextureAsset {
    uint32_t Width=0, Height=0;
    TextureAssetColorFormat ColorFormat=TextureAssetColorFormat::Rgba32;
    TextureAssetAlphaPrecision AlphaPrecision=TextureAssetAlphaPrecision::A8;
    Array<uint8_t>* Colors=nullptr;
    Array<uint8_t>* PaletteColors=nullptr;
};
