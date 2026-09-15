#include "platform/n64/Nintendo64ContentStreamSource.hpp"

#if HELENGINE_N64_HAS_GENERATED_CORE

#include <climits>
#include <dragonfs.h>
#include <stdexcept>
#include <vector>

#include "system/io/file-stream.hpp"

namespace helengine::n64 {
    Nintendo64ContentStreamSource::Nintendo64ContentStreamSource(std::string contentRootPath)
        : ContentRootPath(NormalizeRootPath(contentRootPath)) {
    }

    Stream* Nintendo64ContentStreamSource::OpenRead(std::string assetPath) {
        return Nintendo64ContentStreamSource::OpenRead(BuildAssetPath(assetPath).c_str());
    }

    bool Nintendo64ContentStreamSource::CanHandlePath(const char* path) {
        return path != nullptr && std::string(path).rfind("rom:/", 0) == 0;
    }

    bool Nintendo64ContentStreamSource::Exists(const char* path) {
        if (!CanHandlePath(path)) {
            return false;
        }
        const std::string normalizedPath = NormalizeRelativePath(path);
        return dfs_rom_size(normalizedPath.c_str()) >= 0;
    }

    FileStream* Nintendo64ContentStreamSource::OpenRead(const char* path) {
        if (!CanHandlePath(path)) {
            throw std::invalid_argument("Nintendo 64 DragonFS bridge requires a rom:/ path.");
        }

        const std::string normalizedPath = NormalizeRelativePath(path);
        const int handle = dfs_open(normalizedPath.c_str());
        if (handle < 0) {
            throw std::runtime_error(std::string("N64 DragonFS open failed: ") + normalizedPath);
        }

        const int fileLength = dfs_size(static_cast<uint32_t>(handle));
        if (fileLength < 0) {
            dfs_close(static_cast<uint32_t>(handle));
            throw std::runtime_error(std::string("N64 DragonFS size failed: ") + normalizedPath);
        }
        if (fileLength > INT_MAX) {
            dfs_close(static_cast<uint32_t>(handle));
            throw std::runtime_error(std::string("N64 DragonFS file is too large: ") + normalizedPath);
        }

        std::vector<uint8_t> bytes(static_cast<std::size_t>(fileLength));
        const int bytesRead = fileLength == 0
            ? 0
            : dfs_read(bytes.data(), 1, fileLength, static_cast<uint32_t>(handle));
        const int closeResult = dfs_close(static_cast<uint32_t>(handle));
        if (bytesRead != fileLength || closeResult < 0) {
            throw std::runtime_error(std::string("N64 DragonFS read failed: ") + normalizedPath);
        }

        return new FileStream(bytes.data(), bytes.size());
    }

    std::string Nintendo64ContentStreamSource::NormalizeRootPath(const std::string& value) {
        if (value.empty() || value != "rom:/") {
            throw std::invalid_argument("N64 content root must be rom:/.");
        }
        return value;
    }

    std::string Nintendo64ContentStreamSource::NormalizeRelativePath(const std::string& value) {
        if (value.empty()) {
            throw std::invalid_argument("N64 DragonFS path is required.");
        }

        std::string normalized = value;
        for (char& character : normalized) {
            if (character == '\\') {
                character = '/';
            }
        }
        if (normalized.rfind("rom:/", 0) != 0) {
            throw std::invalid_argument("N64 DragonFS path must start with rom:/.");
        }

        const std::string relativePart = normalized.substr(5);
        if (relativePart.empty() || relativePart.front() == '/') {
            throw std::invalid_argument("N64 DragonFS path must name a relative file.");
        }
        std::size_t segmentStart = 0;
        while (segmentStart <= relativePart.size()) {
            std::size_t segmentEnd = relativePart.find('/', segmentStart);
            if (segmentEnd == std::string::npos) {
                segmentEnd = relativePart.size();
            }
            const std::string segment = relativePart.substr(segmentStart, segmentEnd - segmentStart);
            if (segment.empty() || segment == "." || segment == "..") {
                throw std::invalid_argument("N64 DragonFS path contains an invalid segment.");
            }
            if (segmentEnd == relativePart.size()) {
                break;
            }
            segmentStart = segmentEnd + 1;
        }
        return "rom:/" + relativePart;
    }

    std::string Nintendo64ContentStreamSource::BuildAssetPath(const std::string& assetPath) const {
        std::string normalized = assetPath;
        for (char& character : normalized) {
            if (character == '\\') {
                character = '/';
            }
        }
        if (normalized.rfind("rom:/", 0) == 0) {
            return NormalizeRelativePath(normalized);
        }
        return NormalizeRelativePath(ContentRootPath + normalized);
    }
}

#endif
