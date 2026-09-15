.DEFAULT_GOAL := all

BUILD_DIR := build
SOURCE_DIR := src
HELENGINE_N64_GENERATED_CORE_ROOT ?= generated/core
HELENGINE_N64_MODE ?= diagnostic

LIBDRAGON_PREVIEW := 2
N64_ROM_TITLE := "Helengine N64"
N64_ROM_SAVETYPE := none
N64_ROM_EXPANSIONPAK := unused

include $(N64_INST)/include/n64.mk
include $(N64_INST)/include/t3d.mk

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
ifneq ($(wildcard $(HELENGINE_N64_GENERATED_CORE_ROOT)/*.cpp),)
OBJECTS += $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(wildcard $(HELENGINE_N64_GENERATED_CORE_ROOT)/*.cpp))
endif
else ifneq ($(HELENGINE_N64_MODE),diagnostic)
$(error HELENGINE_N64_MODE must be diagnostic or authored)
endif

all: helengine_n64.z64

$(BUILD_DIR)/helengine_n64.elf: $(OBJECTS)

-include $(OBJECTS:.o=.d)

.PHONY: all clean

clean:
	rm -rf build helengine_n64.z64



