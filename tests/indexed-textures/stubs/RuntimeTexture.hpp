#pragma once
class RuntimeTexture {
public:
    int Width=0, Height=0;
    bool IsEngineOwned=false;
    virtual ~RuntimeTexture() = default;
    virtual void Dispose() {}
};
