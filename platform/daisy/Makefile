# SPDX-License-Identifier: GPL-3.0-only
# Andamento: Tessera's core on a Daisy Seed (platform/daisy/README.md).
#   make LIBDAISY_DIR=~/src/libDaisy          build/tessera_daisy.bin (and .elf, .map)
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
C_INCLUDES = -I$(GEN) -I$(TESSERA)/firmware/src -I.
C_DEFS = -DMELODEE_USB_HOST=0 -DMELODEE_USB_AUDIO=0 -DMELODEE_ACCESS=0
# (Tessera's warnings are checked by its own builds)
CFLAGS += -w

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# the generated headers, as tools/build.py makes them, with the tables at the Daisy's 48 kHz
GEN_HEADERS = $(GEN)/melodee_tables.h $(GEN)/melodee_fm6.h $(GEN)/pid_help.h $(GEN)/x0x_drum_tables.h
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
