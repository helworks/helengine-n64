#pragma once

#include <cstdint>

extern "C" int dfs_rom_size(const char* path);
extern "C" int dfs_open(const char* path);
extern "C" int dfs_size(uint32_t handle);
extern "C" int dfs_read(void* buffer, int size, int count, uint32_t handle);
extern "C" int dfs_close(uint32_t handle);
