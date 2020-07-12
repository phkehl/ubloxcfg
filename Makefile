####################################################################################################
#
# u-blox 9 positioning receivers configuration library and tool
#
# Copyright (c) 2020 Philippe Kehl (flipflip at oinkzwurgl dot org),
# https://oinkzwurgl.org/hacking/ubloxcfg
#
# This program is free software: you can redistribute it and/or modify it under the terms of the
# GNU General Public License as published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program.
# If not, see <https://www.gnu.org/licenses/>.
#
####################################################################################################

# Tools
PERL           := perl
TOUCH          := touch
CP             := cp
MKDIR          := mkdir
TOUCH          := touch
ECHO           := echo
CAT            := cat
RM             := rm
TRUE           := true
FALSE          := false
DOXYGEN        := doxygen
SED            := sed
STRIP          := strip
ZIP            := zip
UNZIP          := unzip

# Toolchain
CC             := gcc
CCWIN64        := x86_64-w64-mingw32-gcc-posix
CXXWIN64       := x86_64-w64-mingw32-g++-posix
CCWIN32        := i686-w64-mingw32-gcc
CXX            := g++
SCANBUILD      := scan-build

# Output and build directories
OUTDIR         := output
BUILDDIR       := build
OBJDIR         := $(BUILDDIR)/obj
OBJDIRWIN      := $(BUILDDIR)/obj_win

# SDL binaries to build cfggui.exe
SDL2WIN        := tmp/SDL2-2.0.12/x86_64-w64-mingw32
#$(shell $(SDL2WIN)/bin/sdl2-config --prefix=$(SDL2WIN) --cflags)
#$(shell $(SDL2WIN)/bin/sdl2-config --prefix=$(SDL2WIN) --libs)
#$(shell $(SDL2WIN)/bin/sdl2-config --prefix=$(SDL2WIN) --static-libs)

# Compilation flags
INCFLAGS       := -I. -Iff -Icfgtool -Iubloxcfg -Icfggui -I3rdparty/stuff -I3rdparty/imgui -I3rdparty/fonts -Ibuild $(shell sdl2-config --cflags)
INCFLAGSWIN    := -I. -Iff -Icfgtool -Iubloxcfg -Icfggui -I3rdparty/stuff -I3rdparty/imgui -I3rdparty/fonts -Ibuild -I$(SDL2WIN)/include/SDL2
CFLAGS         := -g -std=gnu99   -Wall -Wextra -Wformat -Werror -Wpointer-arith -Wundef
CXXFLAGS       := -ggdb3 -std=gnu++11 -Wall -Wextra -Wformat -Werror -Wpointer-arith -Wundef

# Build flags
TOOLLDFLAGS    :=
TOOLLDFLAGSWIN := -lws2_32 -static
GUILDFLAGS     := -lGL -ldl -lpthread $(shell sdl2-config --libs)
#GUILDFLAGSWIN  := -lws2_32 -lgdi32 -lopengl32 -limm32 -L$(SDL2WIN)/lib -lmingw32 -lSDL2main -lSDL2 -mwindows
GUILDFLAGSWIN  := -lws2_32 -lopengl32 -limm32 -L$(SDL2WIN)/lib -lmingw32 -lSDL2main -lSDL2 -mwindows -Wl,--no-undefined -Wl,--dynamicbase -Wl,--nxcompat -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lsetupapi -lversion -luuid -static-libgcc -static
TESTCFLAGS     := -std=c99 -pedantic -Wno-pedantic-ms-format

# Source code
LIBHFILES      := $(sort $(wildcard ubloxcfg/*.h))
LIBCFILES      := $(sort $(wildcard ubloxcfg/*.c))
TOOLHFILES     := $(sort $(wildcard cfgtool/*.h) $(BUILDDIR)/config.h)
TOOLCFILES     := $(sort $(wildcard cfgtool/*.c))
FFHFILES       := $(sort $(wildcard ff/*.h) 3rdparty/stuff/crc24q.h)
FFCFILES       := $(sort $(wildcard ff/*.c) 3rdparty/stuff/crc24q.c)
GUICPPFILES    := $(sort $(wildcard cfggui/*.cpp) $(wildcard ff/*.cpp) $(wildcard 3rdparty/imgui/*.cpp))
GUIHFILES      := $(sort $(wildcard cfggui/*.h) $(wildcard 3rdparty/imgui/*.h) $(BUILDDIR)/config.h)
GUICFILES      := $(sort 3rdparty/imgui/GL/gl3w.c)
ALLSRCFILES    := $(LIBCFILES) $(TOOLCFILES) $(FFCFILES) $(GUICPPFILES) $(GUICFILES) 

# Programs
PROGSRCFILES   := $(sort $(wildcard *.c) $(wildcard *.cpp))

# Current version
VERSION        := $(shell $(SED) -rn '/define CONFIG_VERSION/s/.*"(.+)"/\1/p' config.h.in)

# Verbosity helpers
ifeq ($(VERBOSE),1)
V =
V1 =
V2 =
V12 =
RM += -v
CP += -v
MKDIR += -v
ZIP += -v
UNZIP += -v
else
ZIP += -q
UNZIP += -q
V = @
V1 = > /dev/null
V2 = 2> /dev/null
V12 = 2>&1 > /dev/null
endif

# Fancy output
fancyterm := true
ifeq ($(TERM),dumb)
fancyterm := false
endif
ifeq ($(TERM),)
fancyterm := false
endif
ifeq ($(fancyterm),true)
HLR="\\e[31m"
HLG="\\e[32m"
HLGG="\\e[1\;32m"
HLY="\\e[33m"
HLM="\\e[35m"
HLC="\\e[36m"
HLO="\\e[m"
else
HLR=
HLG=
HLGG=
HLY=
HLM=
HLC=
HLO=
endif

.PHONY: def
def:
	@echo "Make what? Try 'make help'."

####################################################################################################

# Canned recipes for compilation
OBJFILES       :=
OBJFILESWIN    :=

define makeCompileRuleC
#$ (info makeCompileRuleC: $(OBJDIR)/$(subst /,__,$(patsubst %.c,%_c.o,$(1))) ($(1)))
OBJFILES += $(OBJDIR)/$(subst /,__,$(patsubst %.c,%_c.o,$(1)))
$(OBJDIR)/$(subst /,__,$(patsubst %.c,%_c.o,$(1))): $(1) Makefile | $(OBJDIR)
	@echo "$(HLC)$(CC)$(HLO) $(HLG)$$<$(HLO) $(HLM)($$@)$(HLO)"
	$(V)$(CC) -c -o $$@ $$(CFLAGS) $(INCFLAGS) $$< -MD -MF $$(@:%.o=%.d) -MT $$@
endef

define makeCompileRuleCpp
#$ (info makeCompileRuleCpp: $(OBJDIR)/$(subst /,__,$(patsubst %.cpp,%_cpp.o,$(1))) ($(1)))
OBJFILES += $(OBJDIR)/$(subst /,__,$(patsubst %.cpp,%_cpp.o,$(1)))
$(OBJDIR)/$(subst /,__,$(patsubst %.cpp,%_cpp.o,$(1))): $(1) Makefile | $(OBJDIR)
	@echo "$(HLC)$(CXX)$(HLO) $(HLG)$$<$(HLO) $(HLM)($$@)$(HLO)"
	$(V)$(CXX) -c -o $$@ $$(CXXFLAGS) $(INCFLAGS) $$< -MD -MF $$(@:%.o=%.d) -MT $$@
endef

define makeCompileRuleCWin
#$ (info makeCompileRuleCWin: $(OBJDIRWIN)/$(subst /,__,$(patsubst %.c,%_c.o,$(1))) ($(1)))
OBJFILESWIN += $(OBJDIRWIN)/$(subst /,__,$(patsubst %.c,%_c.o,$(1)))
$(OBJDIRWIN)/$(subst /,__,$(patsubst %.c,%_c.o,$(1))): $(1) Makefile | $(OBJDIRWIN)
	@echo "$(HLC)$(CCWIN64)$(HLO) $(HLG)$$<$(HLO) $(HLM)($$@)$(HLO)"
	$(V)$(CCWIN64) -c -o $$@ $$(CFLAGS) $(INCFLAGSWIN) $$< -MD -MF $$(@:%.o=%.d) -MT $$@
endef

define makeCompileRuleCppWin
#$ (info makeCompileRuleCppWin: $(OBJDIRWIN)/$(subst /,__,$(patsubst %.cpp,%_cpp.o,$(1))) ($(1)))
OBJFILESWIN += $(OBJDIRWIN)/$(subst /,__,$(patsubst %.cpp,%_cpp.o,$(1)))
$(OBJDIRWIN)/$(subst /,__,$(patsubst %.cpp,%_cpp.o,$(1))): $(1) Makefile | $(OBJDIRWIN)
	@echo "$(HLC)$(CXXWIN64)$(HLO) $(HLG)$$<$(HLO) $(HLM)($$@)$(HLO)"
	$(V)$(CXXWIN64) -c -o $$@ $$(CXXFLAGS) $(INCFLAGSWIN) $$< -MD -MF $$(@:%.o=%.d) -MT $$@
endef

# Create compile rules and populate $(OBJFILES) list
$(foreach src, $(filter %.c,$(ALLSRCFILES)), $(eval $(call makeCompileRuleC,$(src))))
$(foreach src, $(filter %.cpp,$(ALLSRCFILES)), $(eval $(call makeCompileRuleCpp,$(src))))
$(foreach src, $(filter %.c,$(ALLSRCFILES)), $(eval $(call makeCompileRuleCWin,$(src))))
$(foreach src, $(filter %.cpp,$(ALLSRCFILES)), $(eval $(call makeCompileRuleCppWin,$(src))))

# Load dependency files
ifneq ($(MAKECMDGOALS),clean)
#-include $(DEPFILES)
-include $(sort $(wildcard $(OBJDIR)/*.d) $(wildcard $(OBJDIRWIN)/*.d))
endif

####################################################################################################

.PHONY: help
help:
	@echo "Usage:"
	@echo
	@echo "    make <target> [OUTDIR=...]"
	@echo
	@echo "Available <target>s:"
	@echo
	@echo "    test              Build and run all ubloxcfg tests"
	@echo "    test_m32          Build and run ubloxcfg test with -m32"
	@echo "    test_m64          Build and run ubloxcfg test with -m64"
	@echo "    test.exe          Build all ubloxcfg tests (for Windows)"
	@echo "    test_m32.exe      Build ubloxcfg test with -m32 (for Windows)"
	@echo "    test_m64.exe      Build ubloxcfg test with -m64 (for Windows)"
	@echo "    cfgtool           Build cfgool"
	@echo "    cfgtool.exe       Build cfgool.exe (for Windows)"
	@echo "    doc               Build API and cfgtool documentation"
	@echo "    all               All of the above"
	@echo "    clean             Clean all output"
	@echo "    release           Make release of cfgtool ($(VERSION))"
#	@echo "    cfggui            Build experimental (!) GUI"
	@echo "    scan-build        Run clang static analyzer"
	@echo

# Shortcuts

# used in .github/workflows/main.yml
.PHONY: ci
ci: all

.PHONY: all
all: test cfgtool doc test.exe cfgtool.exe release

.PHONY: doc
doc: $(OUTDIR)/ubloxcfg_html/index.html cfgtool.txt

.PHONY: test
test: test_m32 test_m64

.PHONY: test_m32
test_m32: $(OUTDIR)/test_m32
	@echo "$(HLC)run$(HLO) $(HLG)$<$(HLO)"
	$(V)$^

.PHONY: test_m64
test_m64: $(OUTDIR)/test_m64
	@echo "$(HLC)run$(HLO) $(HLG)$<$(HLO)"
	$(V)$^

.PHONY: test.exe
test.exe: test_m32.exe test_m64.exe

.PHONY: test_m32.exe
test_m32.exe: $(OUTDIR)/test_m32.exe

.PHONY: test_m64.exe
test_m64.exe: $(OUTDIR)/test_m64.exe

.PHONY: cfgtool
cfgtool: $(OUTDIR)/cfgtool

.PHONY: cfgtool.exe
cfgtool.exe: $(OUTDIR)/cfgtool.exe

.PHONY: cfggui
cfggui: $(OUTDIR)/cfggui

.PHONY: cfggui.exe
cfggui.exe: $(OUTDIR)/cfggui.exe

.PHONY: scan-build
scan-build: $(OUTDIR)/scan-build/.done

####################################################################################################

$(OUTDIR):
	$(V)$(MKDIR) -p $(OUTDIR)

$(BUILDDIR):
	$(V)$(MKDIR) -p $(BUILDDIR)

$(OBJDIR):
	$(V)$(MKDIR) -p $(OBJDIR)

$(OBJDIRWIN):
	$(V)$(MKDIR) -p $(OBJDIRWIN)

.PHONY: clean
clean:
	$(V)$(RM) -f core
ifneq ($(OUTDIR),)
	$(V)$(RM) -rf $(OUTDIR)
endif
ifneq ($(OBJDIR),)
	$(V)$(RM) -rf $(OBJDIR)
endif
ifneq ($(OBJDIRWIN),)
	$(V)$(RM) -rf $(OBJDIRWIN)
endif
ifneq ($(BUILDDIR),)
	$(V)$(RM) -rf $(BUILDDIR)
endif

$(BUILDDIR)/config.h: config.h.in config.h.pl Makefile | $(BUILDDIR)
	@echo "$(HLC)generate$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(PERL) config.h.pl < config.h.in > $@.tmp
	$(V)$(CP) $@.tmp $@
	$(V)$(RM) $@.tmp

$(ALLSRCFILES): $(BUILDDIR)/config.h

####################################################################################################
# Config library

# Generate config items info
ubloxcfg/ubloxcfg_gen.c ubloxcfg/ubloxcfg_gen.h: ubloxcfg/ubloxcfg.json ubloxcfg/ubloxcfg_gen.pl Makefile
	@echo "$(HLC)generate$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(PERL) ubloxcfg/ubloxcfg_gen.pl

# Documentation
$(OUTDIR)/ubloxcfg_html/index.html: ubloxcfg/Doxyfile $(LIBHFILES) $(LIBCFILES) Makefile | $(OUTDIR)
	@echo "$(HLC)doxygen$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)( $(CAT) $<; $(ECHO) "OUTPUT_DIRECTORY = $(OUTDIR)"; $(ECHO) "INPUT = $(LIBHFILES) $(LIBCFILES)"; $(ECHO) "QUIET = YES"; ) | $(DOXYGEN) -

#---------------------------------------------------------------------------------------------------
# Library test

$(OUTDIR)/test_m32: test.c $(LIBCFILES) $(LIBHFILES) Makefile | $(OUTDIR)
	@echo "$(HLC)$(CC)$(HLO) $(HLGG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(CC) -m32 $(TESTCFLAGS) $(INCFLAGS) -o $@ $(LIBCFILES) $<

$(OUTDIR)/test_m64: test.c $(LIBCFILES) $(LIBHFILES) Makefile | $(OUTDIR)
	@echo "$(HLC)$(CC)$(HLO) $(HLGG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(CC) -m64 $(TESTCFLAGS) $(INCFLAGS) -o $@ $(LIBCFILES) $<

#---------------------------------------------------------------------------------------------------
# Library test (cross-compile for Windows)

$(OUTDIR)/test_m32.exe: test.c $(LIBCFILES) $(LIBHFILES) Makefile | $(OUTDIR)
	@echo "$(HLC)$(CCWIN32)$(HLO) $(HLGG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(CCWIN32) $(TESTCFLAGS) $(INCFLAGS) -o $@ $(LIBCFILES) $<

$(OUTDIR)/test_m64.exe: test.c $(LIBCFILES) $(LIBHFILES) Makefile | $(OUTDIR)
	@echo "$(HLC)$(CCWIN64)$(HLO) $(HLGG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(CCWIN64) $(TESTCFLAGS) $(INCFLAGS) -o $@ $(LIBCFILES) $<

####################################################################################################
# Configuration tool

TOOLOBJS := $(filter-out $(OBJDIR)/3rdparty__imgui%, $(filter %_c.o,$(OBJFILES)))

$(OUTDIR)/cfgtool: cfgtool.c $(TOOLOBJS) $(TOOLHFILES) Makefile | $(OUTDIR)
	@echo "$(HLC)$(CC)$(HLO) $(HLGG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(CC) $(CFLAGS) $(INCFLAGS) -o $@ $< $(TOOLOBJS) $(TOOLLDFLAGS) -MD -MF $(OBJDIR)/$(notdir $@).d

cfgtool.txt: $(OUTDIR)/cfgtool
	@echo "$(HLC)generate$(HLO) $(HLGG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$^ -H > $(OUTDIR)/$@.tmp
	$(V)$(RM) -f $@
	$(V)$(CP) $(OUTDIR)/$@.tmp $@
	$(V)$(RM) $(OUTDIR)/$@.tmp

#---------------------------------------------------------------------------------------------------
# Cross-compile for Windows

TOOLOBJSWIN := $(filter-out $(OBJDIRWIN)/3rdparty__imgui%, $(filter %_c.o,$(OBJFILESWIN)))

$(OUTDIR)/cfgtool.exe: cfgtool.c $(TOOLOBJSWIN) $(TOOLHFILES) Makefile | $(OUTDIR)
	@echo "$(HLC)$(CCWIN64)$(HLO) $(HLGG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(CCWIN64) $(CFLAGS) $(INCFLAGS) -o $@ $< $(TOOLOBJSWIN) $(TOOLLDFLAGSWIN) -MD -MF $(OBJDIRWIN)/$(notdir $@).d

####################################################################################################
# GUI

GUIOBJS := $(filter-out $(OBJDIR)/cfgtool%, $(OBJFILES))

$(OUTDIR)/cfggui: cfggui.cpp $(GUIOBJS) $(GUIHFILES) Makefile | $(OUTDIR)
	@echo "$(HLC)$(CXX)$(HLO) $(HLGG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(CXX) $(CXXFLAGS) $(INCFLAGS) $(GUIINCFLAGS) -o $@ $< $(GUIOBJS) $(GUILDFLAGS) -MD -MF $(OBJDIR)/$(notdir $@).d

#---------------------------------------------------------------------------------------------------
# Cross-compile for Windows

GUIOBJSWIN := $(filter-out $(OBJDIRWIN)/cfgtool%, $(OBJFILESWIN))

$(OUTDIR)/cfggui.exe: cfggui.cpp $(GUIOBJSWIN) $(GUIHFILES) Makefile | $(OUTDIR)
	@echo "$(HLC)$(CXXWIN64)$(HLO) $(HLGG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(CXXWIN64) $(CXXFLAGS) $(INCFLAGSWIN) $(GUIINCFLAGS) -o $@ $< $(GUIOBJSWIN) $(GUILDFLAGSWIN) -MD -MF $(OBJDIRWIN)/$(notdir $@).d
#	$(V)$(CP) /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll $(OUTDIR)
#	$(V)$(CP) /usr/lib/gcc/x86_64-w64-mingw32/9.3-posix/libgcc_s_seh-1.dll $(OUTDIR)
#	$(V)$(CP) /usr/lib/gcc/x86_64-w64-mingw32/9.3-posix/libstdc++-6.dll $(OUTDIR)
#	$(V)$(CP) $(SDL2WIN)/bin/SDL2.dll $(OUTDIR)

####################################################################################################
# Release

LICENSES_IN := $(wildcard *COPYING*) $(wildcard 3rdparty/stuff/*COPYING*)
LICENSES_OUT := $(addprefix $(OUTDIR)/, $(notdir $(LICENSES_IN)))

RELEASEFILES := $(sort $(OUTDIR)/cfgtool_$(VERSION).bin $(OUTDIR)/cfgtool_$(VERSION).exe \
    $(OUTDIR)/cfgtool_$(VERSION).txt $(LICENSES_OUT))

RELEASEZIP := $(OUTDIR)/ubloxcfg_$(VERSION).zip

.PHONY: release
release: $(RELEASEZIP)

$(OUTDIR)/cfgtool_$(VERSION).bin: $(OUTDIR)/cfgtool Makefile | $(OUTDIR)
	@echo "$(HLC)release$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(CP) $< $@.tmp
	$(V)$(STRIP) $@.tmp
	$(V)$(CP) $@.tmp $@
	$(V)$(RM) $@.tmp

$(OUTDIR)/cfgtool_$(VERSION).exe: $(OUTDIR)/cfgtool.exe Makefile | $(OUTDIR)
	@echo "$(HLC)release$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(CP) $< $@.tmp
	$(V)$(STRIP) $@.tmp
	$(V)$(CP) $@.tmp $@
	$(V)$(RM) $@.tmp

$(OUTDIR)/cfgtool_$(VERSION).txt: cfgtool.txt Makefile | $(OUTDIR)
	@echo "$(HLC)release$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(CP) $< $@

$(LICENSES_OUT): $(LICENSES_IN) Makefile | $(OUTDIR)
	@echo "$(HLC)copying$(HLO) $(HLG)$(LICENSES_IN)$(HLO)"
	$(V)$(CP) $(LICENSES_IN) $(OUTDIR)

$(RELEASEZIP): $(RELEASEFILES)
	@echo "$(HLC)zip$(HLO) $(HLGG)$@$(HLO) $(HLM)($(notdir $^))$(HLO)"
	$(V)$(RM) -f $@
	$(V)( cd $(OUTDIR) && $(ZIP) $(notdir $@) $(notdir $^) )

####################################################################################################
# Static analyzers

scanbuildtargets := cfgtool test cfggui

$(OUTDIR)/scan-build/.done: $(ALLSRCFILES) Makefile | $(OUTDIR)
	@echo "$(HLC)scan-build$(HLO) $(HLG)$(OUTDIR)/scan-build$(HLO) $(HLM)($(scanbuildtargets))$(HLO)"
	$(V)$(SCANBUILD) -o $(OUTDIR)/scan-build -plist-html --exclude 3rdparty $(MAKE) --no-print-directory $(scanbuildtargets)
	$(V)$(TOUCH) $@

####################################################################################################
# eof
