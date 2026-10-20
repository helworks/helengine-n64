#include "platform/n64/Nintendo64RenderManager3D.hpp"
#include "platform/n64/Nintendo64Renderer.hpp"
#include "platform/n64/Nintendo64RuntimeModel.hpp"
#include "platform/n64/Nintendo64RuntimeMaterial.hpp"
#include "platform/n64/Nintendo64RuntimeTexture.hpp"
#include "platform/n64/Nintendo64RenderManager2D.hpp"
#include <libdragon.h>
#include <array>
#include <cstdio>
#include <cstdlib>

namespace helengine::n64 {
bool Nintendo64CubeMesh::Initialize(){return true;}
void Nintendo64CubeMesh::Shutdown(){}
void Nintendo64CubeMesh::Draw()const{}
}
namespace {
void Check(bool condition,const char* reason){if(!condition){std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}}
struct Drawable:IDrawable3D {
 Entity entity;RuntimeModel* model=nullptr;Array<RuntimeMaterial*> materials{1};
 Entity* get_Parent()override{return &entity;}
 RuntimeModel* get_Model()override{return model;}
 Array<RuntimeMaterial*>* get_Materials()override{return &materials;}
};
constexpr std::array<uint32_t,16> colors={0xFF4040FF,0xFF8040FF,0xFFC040FF,0xFFFF40FF,0xC0FF40FF,0x80FF40FF,0x40FF40FF,0x40FF80FF,0x40FFC0FF,0x40FFFFFF,0x40C0FFFF,0x4080FFFF,0x4040FFFF,0x8040FFFF,0xC040FFFF,0xFF40FFFF};
void Verify(bool checkColors){
 rspq_wait();
 Check(deferred_rsp::consumed.size()==192,"all sixteen cubes submit twelve triangles each");
 for(int cube=0;cube<16;cube++)for(int tri=0;tri<12;tri++){
  const auto& result=deferred_rsp::consumed[static_cast<std::size_t>(cube*12+tri)];
  if(checkColors) Check(result.color==colors[static_cast<std::size_t>(cube)],"deferred RSP preserves each original cube color before shared vertex mutation");
  Check(result.x==-4.5f+3.0f*(cube%4)&&result.y==4.5f-3.0f*(cube/4)&&result.z==0,"deferred RSP preserves each cube matrix before shared matrix mutation");
 }
}
void VerifyTextures(){
 using namespace helengine::n64;
 Nintendo64Renderer renderer;Check(renderer.InitializeAuthored(),"renderer initializes for textured draws");
 Nintendo64RenderManager2D render2D;Nintendo64RenderManager3D manager(&renderer,nullptr,&render2D);
 Core core;IContentStreamSource source;core.options.ContentStreamSource=&source;manager.OwnerCore=&core;
 PlatformMaterialAsset asset;asset.RendererFamilyId="n64-standard";asset.TextureRelativePath="textures/cube.hasset";asset.Lit=true;
 auto* texture=new Nintendo64RuntimeTexture();render2D.NextTexture=texture;
 RuntimeMaterial* material=nullptr;
 try{material=manager.BuildMaterialFromCooked(&asset);}catch(InvalidOperationException* error){delete error;Check(false,"N64 opaque materials accept a cooked texture binding");}
 Check(material->ResolvePrimaryTexture()==texture,"material exposes its owned primary texture");
 Check(render2D.LoadedPath==asset.TextureRelativePath&&render2D.LoadedSource==&source,"material loads the exact cooked path through the supplied content source");
 Nintendo64RuntimeModel model;Drawable drawable;drawable.model=&model;drawable.materials[0]=material;
 manager.Visit(&drawable);
 Check(texture_state::uploads==1&&texture_state::surface==texture->Surface(),"textured mesh uploads its real texture surface");
 Check(texture_state::combiner==RDPQ_COMBINER_TEX_SHADE&&(lastDrawFlags&T3D_FLAG_TEXTURED)!=0,"textured mesh modulates texture with lit vertex shading");
 Check((lastDrawFlags&T3D_FLAG_NO_LIGHT)==0&&texture_state::perspective,"textured lit geometry preserves lighting and perspective correction");
 Check(texture_state::tlut==TLUT_RGBA16&&texture_state::palettes==1,"indexed mesh selects and uploads its palette");
 Check(model.textureWidth==64&&model.textureHeight==64,"UV packing uses the bound texture dimensions");
 auto* direct=new Nintendo64RuntimeTexture(32,32,FMT_RGBA16);render2D.NextTexture=direct;
 auto* directMaterial=manager.BuildMaterialFromCooked(&asset);drawable.materials[0]=directMaterial;manager.Visit(&drawable);
 Check(texture_state::uploads==2&&texture_state::tlut==TLUT_NONE,"direct texture clears previous indexed TLUT mode");
 Check(model.textureWidth==32&&model.textureHeight==32,"shared model repacks UVs for different texture dimensions");
 Nintendo64RuntimeMaterial plain(0xFFFFFFFF,false,false);drawable.materials[0]=&plain;manager.Visit(&drawable);
 Check(texture_state::uploads==2&&texture_state::combiner==RDPQ_COMBINER_SHADE&&(lastDrawFlags&T3D_FLAG_TEXTURED)==0,"plain mesh clears texturing without a stale upload");
 Check(texture_state::tlut==TLUT_NONE,"plain mesh leaves no indexed texture state");
 manager.ReleaseMaterial(material);manager.ReleaseMaterial(directMaterial);
 Check(Nintendo64RuntimeTexture::live==0&&Nintendo64RuntimeTexture::releases==2,"owned material textures release exactly once on material disposal");
 auto* oversized=new Nintendo64RuntimeTexture();oversized->SetFits(false);render2D.NextTexture=oversized;
 bool rejected=false;try{auto* invalid=manager.BuildMaterialFromCooked(&asset);manager.ReleaseMaterial(invalid);}catch(InvalidOperationException* error){delete error;rejected=true;}
 Check(rejected&&Nintendo64RuntimeTexture::live==0,"material rejects textures exceeding TMEM and releases failed uploads");
 renderer.Shutdown();std::puts("PASS: textured material loading, indexed/direct/plain transitions, UV dimensions and texture lifetime");
}
void VerifyStreamLifetime(){
 using namespace helengine::n64;
 struct Source:IContentStreamSource {Stream* OpenRead(HeCppString)override{return new Stream();}} source;
 Nintendo64Renderer renderer;Nintendo64RenderManager3D manager(&renderer);
 auto* asset=new PlatformMaterialAsset();asset->RendererFamilyId="n64-standard";AssetSerializer::NextAsset=asset;
 auto* material=manager.BuildMaterialFromCooked("material.hasset",&source);
 Check(Stream::live==0&&Stream::disposed==1,"successful cooked asset read closes and deletes its stream");
 manager.ReleaseMaterial(material);AssetSerializer::ThrowOnRead=true;
 bool rejected=false;try{manager.BuildMaterialFromCooked("invalid.hasset",&source);}catch(InvalidOperationException* error){delete error;rejected=true;}
 Check(rejected&&Stream::live==0&&Stream::disposed==2,"failed cooked asset read closes and deletes its stream");
 std::puts("PASS: cooked asset stream disposal on successful and failed reads");
}
}
int main(int argc,char** argv){
 if(argc>1&&std::string(argv[1])=="--stream"){VerifyStreamLifetime();return 0;}
 if(argc>1&&std::string(argv[1])=="--texture"){VerifyTextures();return 0;}
 using namespace helengine::n64;
 Nintendo64Renderer renderer;Check(renderer.InitializeAuthored(),"real renderer initializes");
 Nintendo64RenderManager3D manager(&renderer);
 Nintendo64RuntimeModel sharedModel;
 std::vector<Nintendo64RuntimeMaterial> materials;materials.reserve(16);
 std::array<Drawable,16> drawables{};
 Core core;ICamera camera;core.objects.cameras.data.push_back(&camera);
 for(int i=0;i<16;i++){
  materials.emplace_back(colors[static_cast<std::size_t>(i)],false,true);
  auto& drawable=drawables[static_cast<std::size_t>(i)];drawable.model=&sharedModel;
  drawable.entity.Position={-4.5f+3.0f*(i%4),4.5f-3.0f*(i/4),0};
  drawable.materials[0]=&materials[static_cast<std::size_t>(i)];
  camera.queue.items.push_back(&drawable);core.objects.drawables.data.push_back(&drawable);
 }
 const std::string mode=argc>1?argv[1]:"";
 if(mode=="--draw"){
  manager.OwnerCore=&core;
  try{manager.Draw();}catch(InvalidOperationException* error){std::fprintf(stderr,"FAIL: Draw rejected sixteen valid model drawables: %s\n",error->what());delete error;return 1;}
 }else for(auto& drawable:drawables)manager.Visit(&drawable);
 Verify(mode!="--matrix");renderer.Shutdown();
 std::puts("PASS: real N64 manager and renderer preserve 16 colors and transforms across deferred RSP consumption (192 triangles)");
}
