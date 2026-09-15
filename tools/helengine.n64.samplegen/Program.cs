using helengine;
using helengine.baseplatform.Builders;
using helengine.directx11;
using helengine.editor;
using helengine.n64.builder;
using helengine.vulkan;
using System.Reflection;

if (args.Length < 2 || args.Length > 3) {
    Console.Error.WriteLine("Usage: fixture-generator <project-root> <package-root> [generated-core-root]");
    Environment.ExitCode = 2;
    return;
}

string projectRoot = Path.GetFullPath(args[0]);
string packageRoot = Path.GetFullPath(args[1]);
Directory.CreateDirectory(Path.Combine(projectRoot, "assets"));
Directory.CreateDirectory(packageRoot);
const string sceneId = "Scenes/Authored.helen";
GeneratedSceneComponentAuthoringService authoring = new GeneratedSceneComponentAuthoringService();
SceneAssetReference model = EngineSceneAssetReferenceFactory.CreateCubeModel();
SceneAssetReference material = EngineSceneAssetReferenceFactory.CreateStandardMaterial();
CameraComponent camera = new CameraComponent {
    CameraDrawOrder = 0,
    LayerMask = 0xFF,
    Viewport = new float4(0f, 0f, 320f, 240f),
    NearPlaneDistance = 0.1f,
    FarPlaneDistance = 100f,
    ClearSettings = new CameraClearSettings(true, new float4(0.02f, 0.03f, 0.05f, 1f), true, 1f, false, 0),
    RenderSettings = new CameraRenderSettings {
        DepthPrepassMode = DepthPrepassMode.Auto,
        ShadowDistance = 0f,
        PostProcessTier = PostProcessTier.Disabled
    }
};
SceneAsset scene = new SceneAsset {
    Id = sceneId,
    AuthoringAssetId = "0123456789abcdef0123456789abcdef",
    FormerAuthoringAssetIds = Array.Empty<string>(),
    AssetReferences = new[] { model, material },
    RootEntities = new[] {
        new SceneEntityAsset {
            Id = 1,
            Name = "Camera",
            LayerMask = 0b00000001,
            LocalPosition = new float3(0f, 0f, -6f),
            LocalScale = float3.One,
            LocalOrientation = float4.Identity,
            Components = new[] { new SceneComponentAssetRecord {
                ComponentTypeId = "helengine.CameraComponent",
                ComponentIndex = 0,
                Payload = authoring.CreateCameraPayload(camera)
            }},
            Children = Array.Empty<SceneEntityAsset>()
        },
        new SceneEntityAsset {
            Id = 2,
            Name = "AuthoredMesh",
            LayerMask = 0b00000001,
            LocalPosition = float3.Zero,
            LocalScale = float3.One,
            LocalOrientation = float4.Identity,
            Components = new[] { new SceneComponentAssetRecord {
                ComponentTypeId = "helengine.MeshComponent",
                ComponentIndex = 0,
                Payload = authoring.CreateMeshPayload(model, material)
            }},
            Children = Array.Empty<SceneEntityAsset>()
        }
    },
    SceneSettings = new SceneSettingsAsset()
};
string authoredScenePath = Path.Combine(projectRoot, "assets", sceneId.Replace('/', Path.DirectorySeparatorChar));
Directory.CreateDirectory(Path.GetDirectoryName(authoredScenePath)!);
using (FileStream stream = File.Create(authoredScenePath)) {
    helengine.files.AssetSerializer.Serialize(stream, scene);
}

ShaderBackendRegistry backends = new ShaderBackendRegistry();
backends.Register(new DirectX11ShaderBackend());
backends.Register(new VulkanShaderBackend());
using EditorBuiltInShaderAssetLibrary shaders = new EditorBuiltInShaderAssetLibrary(backends);
Nintendo64PlatformAssetBuilder n64Builder = new Nintendo64PlatformAssetBuilder();
using EditorPlatformBuildScenePackager packager = new EditorPlatformBuildScenePackager(
    projectRoot,
    Array.Empty<IAssetImporterRegistration>(),
    n64Builder.Definition,
    null,
    n64Builder,
    "debug",
    "n64-unlit",
    shaders);
EditorPlatformBuildScenePackagerResult result = packager.Package(new[] { sceneId }, packageRoot);
Console.WriteLine($"authored-scene-bytes={new FileInfo(authoredScenePath).Length}");
Console.WriteLine($"package-root={packageRoot}");
Console.WriteLine($"platform-cook-items={result.PlatformCookWorkItems.Count}");
foreach (string path in Directory.GetFiles(packageRoot, "*", SearchOption.AllDirectories).OrderBy(path => path, StringComparer.OrdinalIgnoreCase)) {
    Console.WriteLine(Path.GetRelativePath(packageRoot, path).Replace('\\', '/') + " " + new FileInfo(path).Length);
}

if (args.Length == 3) {
    string generatedCoreRoot = Path.GetFullPath(args[2]);
    Nintendo64PlatformAssetBuilder n64CoreBuilder = new Nintendo64PlatformAssetBuilder();
    if (!Directory.Exists(generatedCoreRoot)) {
        throw new DirectoryNotFoundException($"Generated Core root must already contain the converted source tree: {generatedCoreRoot}");
    }

    Type regenerationType = typeof(EditorGeneratedCoreRegenerationService);
    MethodInfo discoverMethod = regenerationType.GetMethod(
        "DiscoverAutomaticRuntimeComponentTypesFromCookedScenes",
        BindingFlags.Static | BindingFlags.NonPublic)
        ?? throw new InvalidOperationException("Could not find cooked-scene component discovery method.");
    IReadOnlyList<Type> componentTypes = (IReadOnlyList<Type>)(discoverMethod.Invoke(
        null,
        new object[] { new[] { Path.Combine(packageRoot, "cooked", "scenes", "authored.hasset") }, null })
        ?? throw new InvalidOperationException("Cooked-scene component discovery returned no result."));
    Console.WriteLine("discovered-components=" + string.Join(",", componentTypes.Select(type => type.FullName)));

    MethodInfo emitMethod = regenerationType.GetMethod(
        "EmitGeneratedAutomaticRuntimeComponentDeserializers",
        BindingFlags.Static | BindingFlags.NonPublic)
        ?? throw new InvalidOperationException("Could not find automatic deserializer generation method.");
    emitMethod.Invoke(
        null,
        new object[] { generatedCoreRoot, componentTypes, n64CoreBuilder.Definition, false });

    MethodInfo unityMethod = regenerationType.GetMethod(
        "WriteGeneratedCoreTranslationUnit",
        BindingFlags.Static | BindingFlags.NonPublic)
        ?? throw new InvalidOperationException("Could not find generated unity writer method.");
    unityMethod.Invoke(null, new object[] { generatedCoreRoot });
    string registrationPath = Path.Combine(generatedCoreRoot, "GeneratedRuntimeComponentDeserializerRegistration.cpp");
    string unityPath = Path.Combine(generatedCoreRoot, "helengine_core_unity.cpp");
    Console.WriteLine("generated-core-registration=" + File.ReadAllText(registrationPath).Count(character => character == '\n'));
    Console.WriteLine("generated-core-unity=" + new FileInfo(unityPath).Length);
}
