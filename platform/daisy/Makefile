# SPDX-License-Identifier: GPL-3.0-only
# Andamento: Tessera's core on a Daisy Seed (platform/daisy/README.md).
#   make LIBDAISY_DIR=~/src/libDaisy          build/tessera_daisy.bin (and .elf, .map), and the memory check
#   make program-dfu                           flash it over USB (the Daisy bootloader's DFU window)
# Needs arm-none-eabi-gcc and a built libDaisy (its own `make`). The tables are made here at 48 kHz.

TARGET = tessera_daisy
# the Daisy bootloader runs it from the 8 MB QSPI flash
APP_TYPE ?= BOOT_QSPI
LIBDAISY_DIR ?= ../../../libDaisy
TESSERA = ../..
GEN = build/gen

CPP_SOURCES = main.cpp
C_SOURCES = tessera_core.c
C_INCLUDES = -I$(GEN) -I$(TESSERA)/firmware/src -I$(TESSERA)/platform/core -I.
C_DEFS = -DTESS_TARGET=TESS_T_DAISY -DMELODEE_USB_HOST=0 -DMELODEE_USB_AUDIO=0 -DMELODEE_ACCESS=0
# the engines the FM-1 leaves out for flash, on here (Theo 2026-11-21: what the plugin adds beyond the FM-1 also
# serves Andamento): SID, DRUM's SID KIT and SID DIGI, MACRO (Plaits, C++ below), PHASE (the CZ-1's phase distortion)
C_DEFS += -DMELODEE_SID=1 -DMELODEE_SID_KIT=1 -DMELODEE_PLAITS=1 -DMELODEE_PHASE=1 -DMELODEE_SMF=1
# the LFO's SLOPE waves and TRIG ONCE, its LFO 90 and LFO x2 sources and the LFO RST destination (core.h)
C_DEFS += -DTESS_LFO_SLOPE=1
# (Tessera's warnings are checked by its own builds)
CFLAGS += -w

# libDaisy's QSPI memory map with Tessera's audio path in ITCM and DTCM (tessera_daisy.lds)
ifneq ($(APP_TYPE),BOOT_QSPI)
$(error platform/daisy: tessera_daisy.lds is the BOOT_QSPI memory map; other APP_TYPEs need their own)
endif
LDSCRIPT = tessera_daisy.lds

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# MACRO's C++: Plaits and stmlib (firmware/third_party/mi) and macro_glue.cc, the list and flags in plaits_sources.sh
# (as andamento-fw builds them). libDaisy's rules know only .cpp, so .cc gets its own, into build/mi/. Not libDaisy's
# CPPFLAGS: its forced -include stm32h7xx.h clashes with Plaits' TEST (a register of that name); -fno-math-errno lets
# sqrtf be the FPU's vsqrt. They run from QSPI, not ITCM: itcm_pick.py reads only tessera_core.o
PLAITS_SRC := $(shell sh plaits_sources.sh $(TESSERA))
PLAITS_OBJ := $(patsubst $(TESSERA)/%.cc,$(BUILD_DIR)/mi/%.o,$(PLAITS_SRC))
OBJECTS += $(PLAITS_OBJ)
$(BUILD_DIR)/$(TARGET).elf: $(PLAITS_OBJ)
$(BUILD_DIR)/mi/%.o: $(TESSERA)/%.cc plaits_sources.sh
	@mkdir -p $(dir $@)
	$(CXX) -c $(MCU) $(OPT) $(CPP_STANDARD) -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-math-errno \
	    -fdata-sections -ffunction-sections -w $(shell sh plaits_sources.sh --flags $(TESSERA)) $< -o $@

# ITCM: what fits of the audio path, picked from the compiled core (itcm_pick.py; tessera_daisy.lds includes it)
LDFLAGS += -L$(BUILD_DIR)
$(BUILD_DIR)/$(TARGET).elf: $(BUILD_DIR)/itcm.ld tessera_daisy.lds
$(BUILD_DIR)/itcm.ld: $(BUILD_DIR)/tessera_core.o itcm_pick.py
	python3 itcm_pick.py $< $@

# after the link: nothing left uncopied, and where the audio path landed (memcheck.py)
.PHONY: memcheck
all: memcheck
memcheck: $(BUILD_DIR)/$(TARGET).elf
	python3 memcheck.py $<

# the generated headers, as tools/build.py makes them, with the tables at the Daisy's 48 kHz
GEN_HEADERS = $(GEN)/melodee_tables.h $(GEN)/melodee_fm6.h $(GEN)/pid_help.h $(GEN)/x0x_drum_tables.h $(UI_HEADERS)
# the FM-1's UI (built in, never run: the shared build, platform/core/core_unit.c): its fonts, icons, keycaps and
# inks, as tools/build.py makes them (needs Pillow with Raqm, and fontTools)
UI_HEADERS = $(GEN)/ui_fonts.h $(GEN)/ui_icons.h $(GEN)/ui_keycaps.h $(GEN)/ui_palettes.h
$(UI_HEADERS) &: $(TESSERA)/tools/build.py
	@mkdir -p $(GEN)
	cd $(TESSERA) && python3 -c "import sys; sys.path.insert(0, 'tools'); import build; build.generate()"
	cp $(addprefix $(TESSERA)/build/gen/,$(notdir $(UI_HEADERS))) $(GEN)/
$(BUILD_DIR)/tessera_core.o: $(GEN_HEADERS)
$(GEN)/melodee_tables.h: $(TESSERA)/tools/gen_tables.py
	@mkdir -p $(GEN)
	MELODEE_FS=48000 python3 $< $@
$(GEN)/melodee_fm6.h: $(TESSERA)/tools/gen_fm6_patches.py
	@mkdir -p $(GEN)
	python3 $< $@
$(GEN)/pid_help.h: $(TESSERA)/tools/gen_help.py
	@mkdir -p $(GEN)
	python3 $< $@
$(GEN)/x0x_drum_tables.h: $(TESSERA)/tools/gen_909_tables.py
	@mkdir -p $(GEN)
	python3 $< $@
