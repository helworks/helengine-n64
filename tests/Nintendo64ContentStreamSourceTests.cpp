#include <cstdio>
#include <cstdlib>
#include <string>

#include "dragonfs.h"

namespace {
std::string lastRomSizePath;
std::string lastOpenPath;
int failures = 0;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}
}

extern "C" int dfs_rom_size(const char* path) {
    lastRomSizePath = path == nullptr ? std::string() : std::string(path);
    return 5;
}

extern "C" int dfs_open(const char* path) {
    lastOpenPath = path == nullptr ? std::string() : std::string(path);
    return 7;
}

extern "C" int dfs_size(uint32_t) {
    return 5;
}

extern "C" int dfs_read(void* buffer, int size, int count, uint32_t) {
    if (buffer != nullptr && size > 0 && count > 0) {
        static_cast<unsigned char*>(buffer)[0] = 0x42;
    }
    return size * count;
}

extern "C" int dfs_close(uint32_t) {
    return 0;
}

#include "system/io/file-stream.hpp"

#ifndef HELENGINE_N64_HAS_GENERATED_CORE
#define HELENGINE_N64_HAS_GENERATED_CORE 1
#endif
#include "platform/n64/Nintendo64ContentStreamSource.hpp"

int main() {
    Check(helengine::n64::Nintendo64ContentStreamSource::Exists("rom:/scenes/authored.scene"),
          "Exists reports the staged authored scene");
    Check(lastRomSizePath == "scenes/authored.scene",
          "Exists strips rom:/ before calling dfs_rom_size");

    FileStream* stream = helengine::n64::Nintendo64ContentStreamSource::OpenRead("rom:/scenes/authored.scene");
    Check(stream != nullptr, "OpenRead returns a stream for the staged authored scene");
    Check(lastOpenPath == "scenes/authored.scene",
          "OpenRead strips rom:/ before calling dfs_open");
    delete stream;

    if (failures != 0) {
        std::fprintf(stderr, "%d content-source test(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    std::puts("n64-content-source-tests: PASS");
    return EXIT_SUCCESS;
}
