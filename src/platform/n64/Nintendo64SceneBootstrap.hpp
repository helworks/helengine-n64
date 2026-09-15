#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

class RuntimeSceneCatalog;

namespace helengine::n64 {
    /// Converts the builder-owned catalog into the generated runtime catalog.
    RuntimeSceneCatalog* BuildNintendo64RuntimeSceneCatalog();
}

#endif
