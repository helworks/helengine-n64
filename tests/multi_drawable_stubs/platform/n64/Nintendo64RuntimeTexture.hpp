#pragma once
#include "GeneratedFixture.hpp"
#include <libdragon.h>
namespace helengine::n64 {
class Nintendo64RuntimeTexture:public ::RuntimeTexture {
 surface_t surface;
 bool disposed=false;
public:
 static inline int live=0,releases=0;
 Nintendo64RuntimeTexture(uint16_t width=64,uint16_t height=64,tex_format_t format=FMT_CI4):surface{width,height,format,true}{live++;}
 ~Nintendo64RuntimeTexture()override{live--;}
 const surface_t* Surface()const{return disposed?nullptr:&surface;}
 int get_Width()const{return surface.width;}
 int get_Height()const{return surface.height;}
 bool IsIndexed()const{return surface.format!=FMT_RGBA16;}
 void Bind(bool=true)const{rdpq_mode_tlut(IsIndexed()?TLUT_RGBA16:TLUT_NONE);if(IsIndexed())texture_state::palettes++;}
 void Dispose()override{if(!disposed){disposed=true;releases++;}}
 void SetFits(bool fits){surface.fits=fits;}
};
}
