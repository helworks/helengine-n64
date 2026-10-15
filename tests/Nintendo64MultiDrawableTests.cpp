#include "platform/n64/Nintendo64RenderManager3D.hpp"
#include "platform/n64/Nintendo64Renderer.hpp"
#include "platform/n64/Nintendo64RuntimeModel.hpp"
#include "platform/n64/Nintendo64RuntimeMaterial.hpp"
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
}
int main(int argc,char** argv){
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
