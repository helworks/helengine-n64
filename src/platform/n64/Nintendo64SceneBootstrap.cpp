#include "platform/n64/Nintendo64SceneBootstrap.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <cstddef>

#include "runtime/array.hpp"
#include "RuntimeSceneCatalog.hpp"
#include "RuntimeSceneCatalogEntry.hpp"
#include "n64_runtime_scene_catalog.hpp"

namespace helengine::n64 {
    RuntimeSceneCatalog* BuildNintendo64RuntimeSceneCatalog() {
        std::size_t sceneCount = 0;
        const HERuntimeSceneCatalogEntry* entries = he_runtime_scene_catalog_entries(&sceneCount);
        if (entries == nullptr || sceneCount == 0) {
            return nullptr;
        }

        Array<RuntimeSceneCatalogEntry*>* catalogEntries = new Array<RuntimeSceneCatalogEntry*>(static_cast<int32_t>(sceneCount));
        for (std::size_t index = 0; index < sceneCount; index++) {
            const HERuntimeSceneCatalogEntry& sourceEntry = entries[index];
            (*catalogEntries)[static_cast<int32_t>(index)] = new RuntimeSceneCatalogEntry(sourceEntry.SceneId, sourceEntry.CookedRelativePath);
        }

        return new RuntimeSceneCatalog(catalogEntries);
    }
}

#endif
