#!/usr/bin/make -f
# ROSS VU // ANALOG ENERGY METER

NAME = ROSSVU

FILES_DSP = RossVUPlugin.cpp
FILES_UI  = RossVUUI.cpp

UI_TYPE = opengl

include ../DPF/Makefile.plugins.mk

BUILD_CXX_FLAGS += -O2 -ffast-math

TARGETS = vst3 clap lv2 jack

all: $(TARGETS)

# Build cruzado para Windows x86-64 (mingw). Sai em ../bin-win.
# BUILD_DIR_SUFFIX separa os objetos e o libdgl do build Linux.
# O DPF nao poe aspas em TARGET_DIR nas regras dele, entao um caminho com espaco
# (como "Central Ricardo Rossati") quebra o make. Monta num staging sem espaco
# e copia depois.
WIN_STAGE = $(HOME)/.cache/rossvu-win

windows:
	@rm -rf "$(WIN_STAGE)"
	$(MAKE) CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++ \
	        AR=x86_64-w64-mingw32-ar BUILD_DIR_SUFFIX=-win \
	        DPF_TARGET_DIR=$(WIN_STAGE) vst3 clap jack
	@rm -rf "../bin-win" && mkdir -p "../bin-win"
	@cp -r "$(WIN_STAGE)/." "../bin-win/"
	@echo "Windows pronto em ../bin-win"

.PHONY: windows
