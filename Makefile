.DEFAULT_GOAL := all

BUILD_DIR := build
SOURCE_DIR := src
HELENGINE_N64_GENERATED_CORE_ROOT ?= generated/core
HELENGINE_N64_MODE ?= diagnostic
HELENGINE_N64_FILESYSTEM_ROOT ?= filesystem

ifeq ($(HELENGINE_N64_MODE),authored)
# n64.mk evaluates N64_MKDFS_ROOT when its %.dfs recipe runs.  Keep the
# source tree explicit so the builder can mount its manifest-only staging tree
# into the Docker build without copying assets into the repository checkout.
N64_MKDFS_ROOT := $(HELENGINE_N64_FILESYSTEM_ROOT)
N64_DFS_ASSET_FILES := $(shell find "$(HELENGINE_N64_FILESYSTEM_ROOT)" -type f -print 2>/dev/null)
endif

LIBDRAGON_PREVIEW := 2
N64_ROM_TITLE := "Helengine N64"
N64_ROM_SAVETYPE := none
N64_ROM_EXPANSIONPAK := unused

include $(N64_INST)/include/n64.mk
include $(N64_INST)/include/t3d.mk

N64_CXXFLAGS += -Isrc

OBJECTS := \
	$(BUILD_DIR)/main.o \
	$(BUILD_DIR)/platform/n64/Nintendo64BootHost.o \
	$(BUILD_DIR)/platform/n64/Nintendo64Video.o \
	$(BUILD_DIR)/platform/n64/Nintendo64CubeMesh.o \
	$(BUILD_DIR)/platform/n64/Nintendo64Renderer.o \
	$(BUILD_DIR)/platform/n64/FrameClock.o

ifeq ($(D),1)
N64_CXXFLAGS += -DHELENGINE_N64_DIAGNOSTIC=1
endif

ifeq ($(HELENGINE_N64_MODE),authored)
N64_CXXFLAGS += -DHELENGINE_N64_AUTHORED=1
ifneq ($(wildcard $(HELENGINE_N64_GENERATED_CORE_ROOT)/Core.hpp),)
ifneq ($(wildcard $(HELENGINE_N64_GENERATED_CORE_ROOT)/helengine_core_unity.cpp),)
N64_CXXFLAGS += -DHELENGINE_N64_HAS_GENERATED_CORE=1
N64_CXXFLAGS += -std=gnu++20
# Generated interface-owned collection entries can point at abstract runtime
# interfaces without virtual destructors; keep the toolchain diagnostic visible
# while allowing the generated Core to compile.
N64_CXXFLAGS += -Wno-error=delete-non-virtual-dtor
N64_CXXFLAGS += -Wno-error=reorder
N64_CXXFLAGS += -Wno-error=sign-compare
N64_CXXFLAGS += -Wno-error=maybe-uninitialized
N64_CXXFLAGS += -I$(HELENGINE_N64_GENERATED_CORE_ROOT)
# The final generated unity unit carries template definitions (for example
# EngineBinaryReader::ReadArray and ContentManager::Load) and the generated
# runtime component deserializers discovered from cooked scenes.  The generic
# generator emits it after cooked-scene discovery; generated_unity.cpp is the
# earlier source tree unity and can have an empty component registration.
N64_GENERATED_CORE_CPP := $(HELENGINE_N64_GENERATED_CORE_ROOT)/helengine_core_unity.cpp $(HELENGINE_N64_GENERATED_CORE_ROOT)/n64_runtime_scene_catalog.cpp
N64_GENERATED_CORE_OBJECTS := $(patsubst $(HELENGINE_N64_GENERATED_CORE_ROOT)/%.cpp,$(BUILD_DIR)/generated-core/%.o,$(N64_GENERATED_CORE_CPP))
OBJECTS += \
	$(BUILD_DIR)/platform/n64/Nintendo64RuntimeHost.o \
	$(BUILD_DIR)/platform/n64/Nintendo64SceneBootstrap.o \
	$(BUILD_DIR)/platform/n64/Nintendo64ContentStreamSource.o \
	$(BUILD_DIR)/platform/n64/Nintendo64RuntimeMaterial.o \
	$(BUILD_DIR)/platform/n64/Nintendo64RuntimeModel.o \
	$(BUILD_DIR)/platform/n64/Nintendo64RenderManager3D.o \
	$(BUILD_DIR)/platform/n64/Nintendo64RenderManager2D.o \
	$(BUILD_DIR)/platform/n64/Nintendo64InputBackend.o
OBJECTS += $(N64_GENERATED_CORE_OBJECTS)
else
$(error HELENGINE_N64_MODE=authored requires final generated helengine_core_unity.cpp; refusing stale generated_unity.cpp fallback)
endif
else
$(error HELENGINE_N64_MODE=authored requires a generated Core.hpp tree; refusing diagnostic fallback)
endif
else ifneq ($(HELENGINE_N64_MODE),diagnostic)
$(error HELENGINE_N64_MODE must be diagnostic or authored)
endif

all: helengine_n64.z64

ifeq ($(HELENGINE_N64_MODE),authored)
# Make the filesystem image an explicit ROM prerequisite.  The declared
# staging tree is removed and recreated by the managed builder, so a deleted
# or renamed artifact cannot survive in an incremental DFS image.
helengine_n64.z64: $(BUILD_DIR)/helengine_n64.dfs
$(BUILD_DIR)/helengine_n64.dfs: $(N64_DFS_ASSET_FILES)
endif

$(BUILD_DIR)/helengine_n64.elf: $(OBJECTS)

$(BUILD_DIR)/generated-core/%.o: $(HELENGINE_N64_GENERATED_CORE_ROOT)/%.cpp
	@mkdir -p $(dir $@)
	@echo "    [CXX] $<"
	$(CXX) -c $(CXXFLAGS) -o $@ $<

-include $(OBJECTS:.o=.d)

.PHONY: all clean

clean:
	rm -rf build helengine_n64.z64



