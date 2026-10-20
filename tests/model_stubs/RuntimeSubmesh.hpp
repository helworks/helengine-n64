#pragma once
#include <cstdint>
enum class ModelPrimitiveTopology { TriangleList };
struct RuntimeSubmesh {
    int32_t IndexStart = 0, IndexCount = 0;
    ModelPrimitiveTopology PrimitiveTopology = ModelPrimitiveTopology::TriangleList;
};
