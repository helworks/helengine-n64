#pragma once
#include <cstdint>
#include <string>
#include <stdexcept>
#include <vector>
using HeCppString=std::string;
using ArgumentNullException=std::runtime_error;
using InvalidOperationException=std::runtime_error;
using NotSupportedException=std::runtime_error;
struct float3 {
 float X=0,Y=0,Z=0;
 float3()=default;float3(float x,float y,float z):X(x),Y(y),Z(z){}
 float3 operator+(const float3& other)const{return {X+other.X,Y+other.Y,Z+other.Z};}
};
struct float4 {
 float X=0,Y=0,Z=0,W=1;
 float4()=default;float4(float x,float y,float z,float w):X(x),Y(y),Z(z),W(w){}
 static float3 RotateVector(float3 vector,float4){return vector;}
};
template<class T> struct Array {
 std::vector<T> data;
 explicit Array(int count=0):data(static_cast<std::size_t>(count)){}
 int get_Length()const{return static_cast<int>(data.size());}
 T& operator[](int index){return data[static_cast<std::size_t>(index)];}
};
template<class T> struct List {
 std::vector<T> data;
 int get_Count()const{return static_cast<int>(data.size());}
 T get_Item(int index){return data[static_cast<std::size_t>(index)];}
 bool empty()const{return data.empty();}
};
class Asset {public:virtual ~Asset()=default;HeCppString Id;HeCppString get_Id(){return Id;}};
class ModelAsset:public Asset {public:Array<float3>* Positions=nullptr;Array<uint16_t>* Indices16=nullptr;void Dispose(){}};
class PlatformMaterialAsset:public Asset {public:HeCppString TextureRelativePath,RendererFamilyId;uint8_t BaseColorR=255,BaseColorG=255,BaseColorB=255,BaseColorA=255;bool DoubleSided=false,Lit=false;};
enum class RuntimeMaterialLightingModel { Unlit,MetalRoughPbr };
class RuntimeTexture {public:virtual ~RuntimeTexture()=default;virtual void Dispose(){};};
class RuntimeMaterial {RuntimeTexture* primary=nullptr;public:virtual ~RuntimeMaterial()=default;virtual void Dispose(){primary=nullptr;};virtual void SetPrimaryTexture(RuntimeTexture* texture){primary=texture;}virtual RuntimeTexture* ResolvePrimaryTexture(){return primary;}RuntimeMaterialLightingModel LightingModel{};bool SupportsNormalMapping=false,SupportsEmissive=false,CastsShadows=false,ReceivesShadows=false;void set_Id(const char*){}};
class RuntimeModel {public:virtual ~RuntimeModel()=default;virtual void Dispose(){};};
class Stream {public:static inline int live=0,disposed=0;Stream(){live++;}virtual ~Stream(){live--;}virtual void Dispose(){disposed++;}};
class IContentStreamSource {public:virtual ~IContentStreamSource()=default;virtual Stream* OpenRead(HeCppString){return nullptr;}};
class AssetSerializer {public:static inline Asset* NextAsset=nullptr;static inline bool ThrowOnRead=false;static Asset* Deserialize(Stream*){if(ThrowOnRead)throw new InvalidOperationException("invalid cooked asset");Asset* asset=NextAsset;NextAsset=nullptr;return asset;}};
class ContentManager {};
template<class Target,class Source> Target* he_cpp_try_cast(Source* source){return dynamic_cast<Target*>(source);}
class Entity {public:float3 Position,Scale{1.5f,1.5f,1.5f};float3 get_Position(){return Position;}float3 get_Scale(){return Scale;}float4 get_Orientation(){return {};}};
class IDrawable3D {public:virtual ~IDrawable3D()=default;virtual Entity* get_Parent()=0;virtual RuntimeModel* get_Model()=0;virtual Array<RuntimeMaterial*>* get_Materials()=0;};
class IRenderVisitor3D {public:virtual ~IRenderVisitor3D()=default;virtual void Visit(IDrawable3D*)=0;};
class IRenderQueue3D {public:std::vector<IDrawable3D*> items;int get_Count(){return static_cast<int>(items.size());}void VisitOrdered(IRenderVisitor3D* visitor){for(auto* item:items)visitor->Visit(item);}};
class CameraClearSettings {public:bool get_ClearColorEnabled(){return false;}float4 get_ClearColor(){return {};}bool get_ClearDepthEnabled(){return false;}float get_ClearDepth(){return 1;}};
class ICamera {public:Entity parent;IRenderQueue3D queue;Entity* get_Parent(){return &parent;}IRenderQueue3D* get_RenderQueue3D(){return &queue;}float4 get_Viewport(){return {0,0,320,240};}float get_NearPlaneDistance(){return .1f;}float get_FarPlaneDistance(){return 100;}CameraClearSettings get_ClearSettings(){return {};}};
class ObjectManager {public:List<ICamera*> cameras;List<IDrawable3D*> drawables;List<ICamera*>* get_Cameras(){return &cameras;}List<IDrawable3D*>* get_Drawables3D(){return &drawables;}};
class SceneManager {public:bool get_IsSceneTransitionActive(){return false;}};
struct CoreInitializationOptions {IContentStreamSource* ContentStreamSource=nullptr;};
class Core {public:ObjectManager objects;SceneManager scenes;CoreInitializationOptions options;ObjectManager* get_ObjectManager(){return &objects;}SceneManager* get_SceneManager(){return &scenes;}CoreInitializationOptions* get_InitializationOptions(){return &options;}};
class RenderManager3D {
public:
 Core* OwnerCore=nullptr;
 virtual ~RenderManager3D()=default;
 virtual RuntimeMaterial* BuildMaterialFromCooked(PlatformMaterialAsset*)=0;
 virtual RuntimeMaterial* BuildMaterialFromCooked(HeCppString,IContentStreamSource*)=0;
 virtual RuntimeMaterial* BuildMaterialFromRawAsset(ContentManager*,HeCppString)=0;
 virtual RuntimeModel* BuildModelFromCooked(HeCppString,IContentStreamSource*)=0;
 virtual RuntimeModel* BuildModelFromRaw(ModelAsset*)=0;
 virtual void Dispose()=0;virtual void Draw()=0;
 virtual void ReleaseMaterial(RuntimeMaterial*)=0;virtual void ReleaseModel(RuntimeModel*)=0;
};
