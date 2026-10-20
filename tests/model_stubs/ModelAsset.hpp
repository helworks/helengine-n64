#pragma once
#include "runtime/array.hpp"
struct float2 { float X = 0, Y = 0; };
struct float3 { float X = 0, Y = 0, Z = 0; };
struct ModelAsset {
    Array<float3>* Positions = nullptr;
    Array<float3>* Normals = nullptr;
    Array<float2>* TexCoords = nullptr;
    Array<uint16_t>* Indices16 = nullptr;
    float3 BoundsMin{}, BoundsMax{};
    ~ModelAsset() { delete Positions; delete Normals; delete TexCoords; delete Indices16; }
};
