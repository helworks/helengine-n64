#pragma once
#include "ModelAsset.hpp"
#include "RuntimeSubmesh.hpp"

// Mirrors generated base ownership/storage only. Tests compile the production N64 model.
class RuntimeModel {
    ModelAsset* Raw = nullptr;
    Array<RuntimeSubmesh*>* Submeshes = nullptr;
public:
    virtual ~RuntimeModel() = default;
    virtual void Dispose() {
        if (Submeshes != nullptr) {
            for (int32_t index = 0; index < Submeshes->get_Length(); ++index) delete (*Submeshes)[index];
            delete Submeshes;
            Submeshes = nullptr;
        }
        delete Raw;
        Raw = nullptr;
    }
    void SetBounds(float3, float3) {}
    void SetRawModelAsset(ModelAsset* value) { delete Raw; Raw = value; }
    ModelAsset* get_RawModelAsset() { return Raw; }
    void SetSubmeshes(Array<RuntimeSubmesh*>* values) {
        Submeshes = new Array<RuntimeSubmesh*>(values->get_Length());
        for (int32_t index = 0; index < values->get_Length(); ++index) (*Submeshes)[index] = (*values)[index];
    }
};
