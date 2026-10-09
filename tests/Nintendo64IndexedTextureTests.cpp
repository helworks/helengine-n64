#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include "platform/n64/Nintendo64RuntimeTexture.hpp"
#include "platform/n64/Nintendo64IndexedTexturePacking.hpp"
#include "platform/n64/Nintendo64TextureBindingCache.hpp"

static int allocations=0, fences=0, uploads=0, lastCount=0;
static rdpq_tlut_t lastMode=TLUT_NONE;
static uint16_t* lastPalette=nullptr;
static bool submitted=false;
static int failAllocation=-1;
void* malloc_uncached_aligned(size_t alignment,size_t size) {
    if (failAllocation==0) return nullptr;
    if (failAllocation>0) --failAllocation;
    void* p=nullptr; if (posix_memalign(&p,alignment,size)) return nullptr;
    ++allocations; return p;
}
void free_uncached(void* p) { assert(!submitted); if(p){--allocations; free(p);} }
surface_t surface_alloc(tex_format_t f,uint16_t w,uint16_t h) {
    const uint16_t stride=f==FMT_CI4 ? (w+1)/2 : (f==FMT_CI8 ? w : w*2);
    return {malloc_uncached_aligned(64,static_cast<size_t>(stride)*h),w,h,stride,f};
}
void surface_free(surface_t* s) { free_uncached(s->buffer); *s={}; }
void rspq_wait() { ++fences; submitted=false; }
void rdpq_mode_tlut(rdpq_tlut_t mode) { lastMode=mode; }
void rdpq_tex_upload_tlut(uint16_t* p,int first,int count) {
    assert(first==0 && reinterpret_cast<uintptr_t>(p)%8==0);
    lastPalette=p; lastCount=count; ++uploads; submitted=true;
}

int main() {
    using namespace helengine::n64;
    std::vector<uint8_t> palette(256*4);
    for(int i=0;i<256;++i){palette[i*4]=palette[i*4+1]=palette[i*4+2]=255;palette[i*4+3]=i;}
    Array<uint8_t> p(palette), indices(std::vector<uint8_t>{0,73,255,127});
    TextureAsset asset; asset.Width=2;asset.Height=2;asset.ColorFormat=TextureAssetColorFormat::Indexed8;
    asset.Colors=&indices;asset.PaletteColors=&p;
    {
        Nintendo64RuntimeTexture t;
        assert(t.Initialize(&asset));assert(t.Surface()->format==FMT_CI8);
        assert(t.TexelBytes()==4 && t.PaletteBytes()==512);
        assert(!memcmp(t.Surface()->buffer,indices.Values.data(),4));
        t.Bind();assert(lastMode==TLUT_IA16 && lastCount==256);
        assert(lastPalette[0]==0xFF00 && lastPalette[73]==0xFF49 && lastPalette[255]==0xFFFF);
        const surface_t* saved=t.Surface();void* savedBuffer=saved->buffer;
        Array<uint8_t> shortIndices(std::vector<uint8_t>{1}); asset.Colors=&shortIndices;
        assert(!t.Initialize(&asset));assert(t.Surface()->buffer==savedBuffer && submitted);
        asset.Colors=&indices;
        p.Values[4]=255;p.Values[5]=0;p.Values[6]=0;
        assert(t.Initialize(&asset));assert(fences==1);
        t.Bind();assert(lastMode==TLUT_RGBA16 && lastPalette[1]==0xF800);
        // Grayscale A8 must not threshold font coverage; colored palettes intentionally use the legacy 5551 policy.
        t.Dispose();assert(!submitted && allocations==0);t.Dispose();assert(allocations==0);
    }
    Array<uint8_t> ci4(std::vector<uint8_t>{0x10,0x32,0x54,0x76,0x08});
    Array<uint8_t> small(std::vector<uint8_t>(16*4,255));
    asset.Width=3;asset.Height=3;asset.ColorFormat=TextureAssetColorFormat::Indexed4;
    asset.Colors=&ci4;asset.PaletteColors=&small;
    {
        Nintendo64RuntimeTexture t;assert(t.Initialize(&asset));assert(t.Surface()->format==FMT_CI4);
        const uint8_t golden[]={0x01,0x20,0x34,0x50,0x67,0x80};
        assert(t.Surface()->stride==2 && t.TexelBytes()==6 && t.PaletteBytes()==32);
        assert(!memcmp(t.Surface()->buffer,golden,sizeof(golden)));
        t.Bind();assert(lastCount==16);
        small.Values.resize(8*4);assert(!t.Initialize(&asset));small.Values.resize(64,255);
        asset.AlphaPrecision=static_cast<TextureAssetAlphaPrecision>(4);assert(!t.Initialize(&asset));
        asset.AlphaPrecision=TextureAssetAlphaPrecision::A8;
    }
    assert(allocations==0);
    // RGBA resets a preceding indexed mode and preserves anonymous font alpha in the legacy path.
    Array<uint8_t> rgba(std::vector<uint8_t>{255,255,255,73});
    asset.Width=1;asset.Height=1;asset.ColorFormat=TextureAssetColorFormat::Rgba32;asset.Colors=&rgba;
    asset.AlphaPrecision=TextureAssetAlphaPrecision::Opaque;
    {Nintendo64RuntimeTexture t;assert(t.Initialize(&asset));t.Bind();assert(lastMode==TLUT_NONE);
        assert(*static_cast<uint16_t*>(t.Surface()->buffer)==0xFFFE);}
    // Full 512 atlas remains indexed in RAM; the SDK blitter supplies its 2048-byte TMEM chunks.
    Array<uint8_t> atlas(std::vector<uint8_t>(512*512,73));
    asset.Width=512;asset.Height=512;asset.ColorFormat=TextureAssetColorFormat::Indexed8;asset.Colors=&atlas;
    asset.PaletteColors=&p;asset.AlphaPrecision=TextureAssetAlphaPrecision::A8;
    {Nintendo64RuntimeTexture t;assert(t.Initialize(&asset));assert(t.TexelBytes()==262144);t.Bind();}
    assert(allocations==0);
    // Allocation failure also releases the first uncached texel allocation without leaking a half-owned object.
    failAllocation=1;{Nintendo64RuntimeTexture t;assert(!t.Initialize(&asset));assert(!t.Surface());}failAllocation=-1;
    assert(allocations==0);
    // Live palettes retain independent addresses across IA -> color -> RGBA -> IA changes.
    p.Values[4]=p.Values[5]=p.Values[6]=255;
    {Nintendo64RuntimeTexture ia,color,direct;
        assert(ia.Initialize(&asset));ia.Bind();uint16_t* iaPointer=lastPalette;
        p.Values[4]=255;p.Values[5]=0;p.Values[6]=0;
        assert(color.Initialize(&asset));color.Bind();uint16_t* colorPointer=lastPalette;
        assert(colorPointer!=iaPointer && lastMode==TLUT_RGBA16);
        TextureAsset directAsset;directAsset.Width=1;directAsset.Height=1;directAsset.Colors=&rgba;
        assert(direct.Initialize(&directAsset));direct.Bind();assert(lastMode==TLUT_NONE);
        int before=uploads;ia.Bind();assert(lastMode==TLUT_IA16 && lastPalette==iaPointer && uploads==before+1);
        assert(iaPointer[73]==0xFF49);color.Bind();assert(lastPalette==colorPointer);
        // This is the production cache instantiated locally by Execute, shared by glyph and sprite blits.
        int start=uploads;
        {Nintendo64TextureBindingCache execution;
            for(int glyph=0;glyph<300;++glyph) execution.Bind(&ia);
            assert(uploads==start+1 && lastMode==TLUT_IA16);
            execution.Bind(nullptr);assert(uploads==start+1);
        }
        start=uploads;
        {Nintendo64TextureBindingCache execution;execution.Bind(&ia);execution.Bind(&color);execution.Bind(&ia);
            assert(uploads==start+3 && lastPalette==iaPointer);}
        start=uploads;
        {Nintendo64TextureBindingCache execution;execution.Bind(&ia);execution.Bind(&direct);
            assert(lastMode==TLUT_NONE);execution.Bind(&ia);assert(uploads==start+2);}
        start=uploads;
        {Nintendo64TextureBindingCache firstExecute;firstExecute.Bind(&ia);}
        {Nintendo64TextureBindingCache nextExecute;nextExecute.Bind(&ia);}
        assert(uploads==start+2);
        start=uploads;ia.Bind();ia.Bind();assert(uploads==start+2); // External callers remain conservative.
    }
    assert(allocations==0);
    const uint8_t validIndex[]={1}, invalidIndex[]={2}, twoEntries[]={0,0,0,0,255,255,255,255};
    assert(Nintendo64IndexedTexturePacking::Validate(validIndex,1,1,false,twoEntries,8,3));
    assert(!Nintendo64IndexedTexturePacking::Validate(invalidIndex,1,1,false,twoEntries,8,3));
    assert(!Nintendo64IndexedTexturePacking::Validate(validIndex,1,1,false,twoEntries,7,3));
    assert(!Nintendo64IndexedTexturePacking::Validate(validIndex,1,1,false,twoEntries,8,4));
    uint16_t words[16];const uint8_t gray[]={31,31,31,73};
    Nintendo64IndexedTexturePacking::ConvertPalette(gray,1,0,true,words,16);assert(words[0]==0x1FFF);
    Nintendo64IndexedTexturePacking::ConvertPalette(gray,1,1,true,words,16);assert(words[0]==0x1F00);
    Nintendo64IndexedTexturePacking::ConvertPalette(gray,1,2,true,words,16);assert(words[0]==0x1F44);
    std::cout<<"N64 indexed production upload/bind/lifetime and packing tests passed\n";
}
