#pragma once

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <string>

#include "IContentStreamSource.hpp"

class FileStream;

namespace helengine::n64 {
    /// Bridges generated-core content reads to the DragonFS `rom:/` mount.
    ///
    /// The static methods are also the custom-file-system hook used by the
    /// generated FileStream implementation.  The instance method is the
    /// normal Core IContentStreamSource path used by cooked asset loaders.
    class Nintendo64ContentStreamSource final : public IContentStreamSource {
    public:
        explicit Nintendo64ContentStreamSource(std::string contentRootPath = "rom:/");

        Stream* OpenRead(std::string assetPath) override;

        static bool CanHandlePath(const char* path);
        static bool Exists(const char* path);
        static FileStream* OpenRead(const char* path);

    private:
        std::string ContentRootPath;

        static std::string NormalizeRootPath(const std::string& value);
        static std::string NormalizeRelativePath(const std::string& value);
        std::string BuildAssetPath(const std::string& assetPath) const;
    };
}

#endif
