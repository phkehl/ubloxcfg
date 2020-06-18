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

# Toolchain
CC             := gcc
CCWIN64        := x86_64-w64-mingw32-gcc
CCWIN32        := i686-w64-mingw32-gcc
CXX            := g++

# Compilation flags
INCFLAGS       := -I. -Iff -Icfgtool -Iubloxcfg -Igui -I3rdparty/stuff -Ibuild
CFLAGS         := -g -std=gnu99   -Wall -Wextra -Wformat -Werror -Wpointer-arith -Wundef
CXXFLAGS       := -g -std=gnu++11 -Wall -Wextra -Wformat -Werror -Wpointer-arith -Wundef

# Build flags
TOOLLDFLAGS    :=
TOOLLDFLAGSWIN := -lpthread -lws2_32 -static
GUILDFLAGS     := -lGL -ldl -lpthread $(shell sdl2-config --libs)
TESTCFLAGS     := -std=c99 -pedantic -Wno-pedantic-ms-format

# Output and build directories
OUTDIR         := output
BUILDDIR       := build

# Source code
LIBHFILES      := $(sort $(wildcard ubloxcfg/*.h))
LIBCFILES      := $(sort $(wildcard ubloxcfg/*.c))
TOOLHFILES     := $(sort $(wildcard cfgtool/*.h) $(BUILDDIR)/config.h)
TOOLCFILES     := $(sort $(wildcard cfgtool/*.c))
FFHFILES       := $(sort $(wildcard ff/*.h) 3rdparty/stuff/crc24q.h)
FFCFILES       := $(sort $(wildcard ff/*.c) 3rdparty/stuff/crc24q.c)
ALLSRCFILES    := $(LIBCFILES) $(TOOLCFILES) $(FFCFILES)

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
else
ZIP += -q
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

OBJDIR         := $(BUILDDIR)/obj
OBJDIRWIN      := $(BUILDDIR)/obj_win

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
#$ (info makeCompileRuleCWin: $(OBJDIR)/$(subst /,__,$(patsubst %.c,%_c.o,$(1))) ($(1)))
OBJFILESWIN += $(OBJDIRWIN)/$(subst /,__,$(patsubst %.c,%_c.o,$(1)))
$(OBJDIRWIN)/$(subst /,__,$(patsubst %.c,%_c.o,$(1))): $(1) Makefile | $(OBJDIRWIN)
	@echo "$(HLC)$(CCWIN64)$(HLO) $(HLG)$$<$(HLO) $(HLM)($$@)$(HLO)"
	$(V)$(CCWIN64) -c -o $$@ $$(CFLAGS) $(INCFLAGS) $$< -MD -MF $$(@:%.o=%.d) -MT $$@
endef

# Create compile rules and populate $(OBJFILES) list
$(foreach src, $(filter %.c,$(ALLSRCFILES)), $(eval $(call makeCompileRuleC,$(src))))
$(foreach src, $(filter %.cpp,$(ALLSRCFILES)), $(eval $(call makeCompileRuleCpp,$(src))))
$(foreach src, $(filter %.c,$(ALLSRCFILES)), $(eval $(call makeCompileRuleCWin,$(src))))

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

$(BUILDDIR)/config.h: config.h.in config.h.pl $(filter-out $(BUILDDIR)/config.h, $(ALLSRCFILES) $(PROGSRCFILES)) Makefile | $(BUILDDIR)
	@echo "$(HLC)generate$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(PERL) config.h.pl < config.h.in > $@.tmp
	$(V)$(CP) $@.tmp $@
	$(V)$(RM) $@.tmp

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
# eof
