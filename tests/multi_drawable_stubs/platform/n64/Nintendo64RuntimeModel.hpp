#pragma once
#include "GeneratedFixture.hpp"
#include "t3d/t3d.h"
namespace helengine::n64 {
class Nintendo64RuntimeModel:public ::RuntimeModel {
 T3DVertPacked vertices[12]{};uint16_t indices[36]{};ModelAsset asset;
public:
 int textureWidth=0,textureHeight=0;
 bool HasTextureCoordinates()const{return true;}
 bool SetTextureDimensions(uint32_t width,uint32_t height){textureWidth=static_cast<int>(width);textureHeight=static_cast<int>(height);return true;}
 bool Initialize(ModelAsset*){return true;}
 ModelAsset* get_RawModelAsset(){return &asset;}
 bool HasNormals(){return true;}
 void SetColor(uint32_t color){for(auto& vertex:vertices){vertex.rgbaA=color;vertex.rgbaB=color;}}
 T3DVertPacked* Vertices(){return vertices;}
 const uint16_t* Indices(){return indices;}
 uint32_t VertexCount(){return 24;}
 uint32_t IndexCount(){return 36;}
};
}
