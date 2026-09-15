.DEFAULT_GOAL := all

BUILD_DIR := build
SOURCE_DIR := src

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

all: helengine_n64.z64

$(BUILD_DIR)/helengine_n64.elf: $(OBJECTS)

-include $(OBJECTS:.o=.d)

.PHONY: all clean

clean:
	rm -rf build helengine_n64.z64



