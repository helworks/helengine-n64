#pragma once
#include "GeneratedFixture.hpp"
namespace helengine::n64 {struct Nintendo64RenderManager2D {
 RuntimeTexture* NextTexture=nullptr;
 HeCppString LoadedPath;
 IContentStreamSource* LoadedSource=nullptr;
 void DrawCamera(ICamera*) {}
 RuntimeTexture* BuildTextureFromCooked(HeCppString path,IContentStreamSource* source){LoadedPath=path;LoadedSource=source;auto* result=NextTexture;NextTexture=nullptr;return result;}
 void ReleaseTexture(RuntimeTexture* texture){if(texture){texture->Dispose();delete texture;}}
};}
