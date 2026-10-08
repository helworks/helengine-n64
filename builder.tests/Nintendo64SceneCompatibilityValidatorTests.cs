using helengine.baseplatform.Manifest;
using helengine.n64.builder;

namespace helengine.n64.builder.tests;

public sealed class Nintendo64SceneCompatibilityValidatorTests {
    [Fact]
    public void Validate_AllowsSplashMenuAndLoadingScenesWhenStartupIsIncluded() {
        PlatformBuildManifest manifest = new(
            1,
            "project",
            "1.0.0",
            "1.0.0",
            "n64",
            "1.0.0",
            "HelenOfCodeSplash",
            [
                new PlatformBuildScene("HelenOfCodeSplash", "Splash", "scenes/HelenOfCodeSplash.helen", [], []),
                new PlatformBuildScene("SceneLoadingScreen", "Loading", "scenes/SceneLoadingScreen.helen", [], []),
                new PlatformBuildScene("DemoDiscMainMenu", "Main Menu", "scenes/DemoDiscMainMenu.helen", [], [])
            ],
            [],
            [],
            [],
            [],
            new PlatformContainerWritePlan("n64", []),
            [],
            PlatformBuildRuntimeFeatureManifest.Empty);

        new Nintendo64SceneCompatibilityValidator().Validate(manifest);
    }
}
