########################################################################################################################
#
# u-blox 9 positioning receivers configuration library and tool
#
# Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org),
# https://oinkzwurgl.org/hacking/ubloxcfg
#
# This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program.
# If not, see <https://www.gnu.org/licenses/>.
#
########################################################################################################################

.PHONY: default
default:
	@echo "Make what? Try 'make help'!"

########################################################################################################################

# Directory for intermediate build files (e.g. object files)
BUILDDIR              := build

# Directory for build output (e.g. executables)
OUTPUTDIR             := output

# Cross-compile for Windows using mingw-w64 (https://mingw-w64.org)
ifeq ($(WIN),64)
CC              = x86_64-w64-mingw32-gcc-posix
CXX             = x86_64-w64-mingw32-gcc-posix
LD              = x86_64-w64-mingw32-gcc-posix
NM              = x86_64-w64-mingw32-nm-posix
OBJCOPY         = x86_64-w64-mingw32-objcopy
OBJDUMP         = x86_64-w64-mingw32-objdump
RANLIB          = x86_64-w64-mingw32-ranlib-posix
READELF         = true
STRINGS         = x86_64-w64-mingw32-strings
STRIP           = x86_64-w64-mingw32-strip
EXE             = .exe
endif

# Compile in msys2 mingw
ifneq ($(MSYSTEM),)
  EXE = .exe
  ifneq ($(MSYSTEM),MINGW64)
    $(error Expecting MSYSTEM=MINGW64. You have $(MSYSTEM))
  endif
endif

include build.mk

# Current program version
VERSION               := $(shell $(SED) -rn '/define CONFIG_VERSION/s/.*"(.+)"/\1/p' config.h.in)

# ubloxcfg library
CFILES_ubloxcfg       := $(wildcard ubloxcfg/*.c)
CFLAGS_library        := -fPIC
LDFLAGS_library       := -shared -lm
$(CFILES_ubloxcfg): $(BUILDDIR)/config.h

# ff library souces
CFILES_ff             := $(wildcard ff/*.c)
CXXFILES_ff           := $(wildcard ff/*.cpp)
$(CFILES_ff): $(BUILDDIR)/config.h

# Toolchain flags
CFLAGS_all            := -Wall -Wextra -Werror -Wshadow
CXXFLAGS_all          := -Wall -Wextra -Werror -Wshadow
LDFLAGS_all           :=

CFLAGS_release        := -DFF_BUILD_RELEASE -DNDEBUG -O3 -g
CXXFLAGS_release      := -DFF_BUILD_RELEASE -DNDEBUG -O3 -g
LDFLAGS_release       := -g

CFLAGS_debug          := -DFF_BUILD_DEBUG -Og -ggdb
CXXFLAGS_debug        := -DFF_BUILD_DEBUG -Og -ggdb
LDFLAGS_debug         := -ggdb
ifeq ($(WIN),)
CFLAGS_debug          += -rdynamic
CXXFLAGS_debug        += -rdynamic
LDFLAGS_debug         += -rdynamic
endif


# test (ubloxcfg)
CFILES_test_m32       := test/test_ubloxcfg.c
CFLAGS_test_m32       := -std=c99 -pedantic -Wno-pedantic-ms-format -m32
LDFLAGS_test_m32      := -m32
CFILES_test_m64       := test/test_ubloxcfg.c
CFLAGS_test_m64       := -std=c99 -pedantic -Wno-pedantic-ms-format -m64
LDFLAGS_test_m64      := -m64
$(CFILES_test_m32): $(BUILDDIR)/config.h
$(CFILES_test_m64): $(BUILDDIR)/config.h

# cfgtool
CFILES_cfgtool        := $(wildcard cfgtool/*.c)
CFLAGS_cfgtool        := -std=gnu99 -Wformat -Wpointer-arith -Wundef
LDFLAGS_cfgtool       := -lm
ifeq ($(WIN),64)
LDFLAGS_cfgtool       += -lws2_32 -static
endif
$(CFILES_cfgtool): $(BUILDDIR)/config.h
$(CFILES_cfgtool): $(BUILDDIR)/config.h

# cfggui
CXXFILES_cfggui       := $(wildcard cfggui/*.cpp) $(wildcard cfggui/*/*.cpp) $(wildcard ff/*.cpp)
CXXFILES_cfggui       += $(wildcard 3rdparty/imgui/*.cpp) $(wildcard 3rdparty/implot/*.cpp) $(wildcard 3rdparty/stuff/*.cpp)
CFILES_cfggui         := $(wildcard 3rdparty/stb/*.c) 3rdparty/stuff/tetris.c  3rdparty/stuff/gl3w.c $(wildcard 3rdparty/nanovg/*.c)
CFLAGS_cfggui         := -std=gnu99 -Wformat -Wpointer-arith -Wundef
CXXFLAGS_cfggui       := -std=gnu++17 -Wformat -Wpointer-arith -Wundef -I3rdparty/fonts
LDFLAGS_cfggui        := -lm -lpthread -lstdc++fs -lstdc++ -ldl
CXXFLAGS_cfggui       += $(shell pkg-config --cflags glfw3 freetype2 zlib glm 2>/dev/null)
LDFLAGS_cfggui        += $(shell pkg-config --libs   glfw3 freetype2 zlib glm 2>/dev/null)
CXXFLAGS_cfggui       += $(shell curl-config --cflags 2>/dev/null)
LDFLAGS_cfggui        += $(shell curl-config --libs 2>/dev/null)
LDFLAGS_cfggui        += -lGL
#-lGLU -ldl
$(CFILES_cfggui): $(BUILDDIR)/config.h
$(CXXFILES_cfggui): $(BUILDDIR)/config.h

# Binaries, makeTarget: name, .c/.cpp files, CFLAGS, CXXFLAGS, LDFLAGS -- The final CFLAGS, CXXFLAGS and LDFLAGS will be $(CFLAGS) etc. from the environment, + those given here
$(eval $(call makeTarget, test_m32-release$(EXE), $(CFILES_test_m32) $(CFILES_ubloxcfg),                                 $(CFLAGS_all) $(CFLAGS_release) $(CFLAGS_test_m32),                                                       , $(LDLFAGS_all) $(LDFLAGS_release) $(LDFLAGS_test_m32)))
$(eval $(call makeTarget, test_m32-debug$(EXE),   $(CFILES_test_m32) $(CFILES_ubloxcfg),                                 $(CFLAGS_all) $(CFLAGS_debug)   $(CFLAGS_test_m32),                                                       , $(LDLFAGS_all) $(LDFLAGS_debug)   $(LDFLAGS_test_m32)))
$(eval $(call makeTarget, test_m64-release$(EXE), $(CFILES_test_m64) $(CFILES_ubloxcfg),                                 $(CFLAGS_all) $(CFLAGS_release) $(CFLAGS_test_m64),                                                       , $(LDLFAGS_all) $(LDFLAGS_release) $(LDFLAGS_test_m64)))
$(eval $(call makeTarget, test_m64-debug$(EXE),   $(CFILES_test_m64) $(CFILES_ubloxcfg),                                 $(CFLAGS_all) $(CFLAGS_debug)   $(CFLAGS_test_m64),                                                       , $(LDLFAGS_all) $(LDFLAGS_debug)   $(LDFLAGS_test_m64)))
$(eval $(call makeTarget, cfgtool-release$(EXE),  $(CFILES_cfgtool)  $(CFILES_ubloxcfg) $(CFILES_ff) $(CFILES_cfgtool),  $(CFLAGS_all) $(CFLAGS_release) $(CFLAGS_cfgtool),                                                        , $(LDLFAGS_all) $(LDFLAGS_release) $(LDFLAGS_cfgtool)))
$(eval $(call makeTarget, cfgtool-debug$(EXE),    $(CFILES_cfgtool)  $(CFILES_ubloxcfg) $(CFILES_ff) $(CFILES_cfgtool),  $(CFLAGS_all) $(CFLAGS_debug)   $(CFLAGS_cfgtool),                                                        , $(LDLFAGS_all) $(LDFLAGS_debug)   $(LDFLAGS_cfgtool)))
ifeq ($(WIN),)
$(eval $(call makeTarget, cfggui-release$(EXE),   $(CFILES_cfggui)   $(CXXFILES_cfggui) $(CFILES_ubloxcfg) $(CFILES_ff), $(CFLAGS_all) $(CFLAGS_release) $(CFLAGS_cfggui),   $(CXXFLAGS_all) $(CXXFLAGS_release) $(CXXFLAGS_cfggui), $(LDFLAGS_all) $(LDFLAGS_release) $(LDFLAGS_cfggui)))
$(eval $(call makeTarget, cfggui-debug$(EXE),     $(CFILES_cfggui)   $(CXXFILES_cfggui) $(CFILES_ubloxcfg) $(CFILES_ff), $(CFLAGS_all) $(CFLAGS_release) $(CFLAGS_cfggui),   $(CXXFLAGS_all) $(CXXFLAGS_debug)   $(CXXFLAGS_cfggui), $(LDFLAGS_all) $(LDFLAGS_debug)   $(LDFLAGS_cfggui)))
endif
$(eval $(call makeTarget, libubloxcfg.so,         $(CFILES_ubloxcfg) $(CFILES_ff),                                       $(CFLAGS_all) $(CFLAGS_release) $(CFLAGS_library),                                                        , $(LDFLAGS_ALL) $(LDFLAGS_release) $(LDFLAGS_library)))
########################################################################################################################

# Make config.h
$(BUILDDIR)/config.h: config.h.in config.h.pl Makefile | $(BUILDDIR)
	@echo "$(HLY)*$(HLO) $(HLC)GEN$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(PERL) config.h.pl < config.h.in > $@.tmp
	$(V)$(CP) $@.tmp $@
	$(V)$(RM) $@.tmp

# Generate config items info
CFG_JSONC := $(sort $(wildcard ubloxcfg/ubloxcfg-*.jsonc))
ubloxcfg/ubloxcfg_gen.c ubloxcfg/ubloxcfg_gen.h: $(CFG_JSONC) ubloxcfg/ubloxcfg_gen.pl Makefile
	@echo "$(HLY)*$(HLO) $(HLC)GEN$(HLO) $(HLG)$@$(HLO) $(HLM)($(CFG_JSONC))$(HLO)"
	$(V)$(PERL) ubloxcfg/ubloxcfg_gen.pl ubloxcfg/ubloxcfg_gen $(CFG_JSONC)

ubloxcfg/ubloxcfg.c: ubloxcfg/ubloxcfg_gen.c ubloxcfg/ubloxcfg_gen.h

# Documentation
$(OUTPUTDIR)/ubloxcfg_html/index.html: ubloxcfg/Doxyfile $(LIBHFILES) $(LIBCFILES) Makefile | $(OUTPUTDIR)
	@echo "$(HLY)*$(HLO) $(HLC)doxygen$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)( $(CAT) $<; $(ECHO) "OUTPUT_DIRECTORY = $(OUTPUTDIR)"; $(ECHO) "QUIET = YES"; ) | $(DOXYGEN) -

########################################################################################################################

# Used in .github/workflows/main.yml
.PHONY: ci
ci: all

# Make everything
.PHONY: all
all: test_m32-release test_m64-release cfgtool-release cfggui-release release cfgtool.txt

# Some shortcuts
test_m32: test_m32-release
test_m64: test_m64-release
test: test_m32 test_m64
	$(OUTPUTDIR)/test_m32-release
	$(OUTPUTDIR)/test_m64-release
.PHONY: cfgtool
cfgtool: cfgtool-release
.PHONY: cfggui
cfggui: cfggui-release
.PHONY: doc
doc: $(OUTPUTDIR)/ubloxcfg_html/index.html

########################################################################################################################

# Help screen
.PHONY: help
help:
	@echo "Usage:"
	@echo
	@echo "    make <target> ... [VERBOSE=0|1]"
	@echo
	@echo "Where possible <target>s are:"
	@echo
	@echo "    clean           Clean all ($(OUTPUTDIR) and $(BUILDDIR) directories)"
	@echo "    all             Build (mostly) everything"
	@echo "    <prog>-<build>  Make binary, <prog> is cfgtool, cfggui, ... and <build> is release, debug"
	@echo "    test            Build and run tests"
	@echo "    doc             Build HTML docu of the ubloxcfg library"
	@echo "    debugmf         Show some Makefile variables"
	@echo "    scan-build      Run scan-build"
	@echo "    cfggui-valgrind Run cfggui with valgrind"
	@echo
	@echo "To cross-compile for windows, use 'make <prog>-<build>.exe WIN=64'."
	@echo "Note that not all programs cross-compile for Windoze!"
	@echo "Some stuff may compile (and even work) using native compilation in mingw-w64."
	@echo
	@echo "See README.md for more details"
	@echo

########################################################################################################################
# CI targets, see Jenkinsfile

.PHONY: ci-build
ci-build: all

.PHONY: ci-test
ci-test: test

####################################################################################################
# Release (cfgtool only, no gui yet...)

RELEASEFILES := $(sort $(OUTPUTDIR)/cfgtool_$(VERSION).bin  \
    $(OUTPUTDIR)/cfgtool_$(VERSION).txt $(LICENSES_OUT))

RELEASEZIP := $(OUTPUTDIR)/ubloxcfg_$(VERSION).zip

.PHONY: release
release: $(RELEASEZIP)

$(OUTPUTDIR)/cfgtool_$(VERSION).bin: $(OUTPUTDIR)/cfgtool-release Makefile | $(OUTPUTDIR)
	@echo "$(HLY)*$(HLO) $(HLC)REL$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(CP) $< $@.tmp
	$(V)$(STRIP) $@.tmp
	$(V)$(CP) $@.tmp $@
	$(V)$(RM) $@.tmp
	$(V)$(CP) ff/LICENSE                      $(OUTPUTDIR)/ff_LICENSE
	$(V)$(CP) ubloxcfg/LICENSE                $(OUTPUTDIR)/ubloxcfg_LICENSE

cfgtool.txt: $(OUTPUTDIR)/cfgtool-release
	@echo "$(HLY)*$(HLO) $(HLC)GEN$(HLO) $(HLGG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$^ -H > $(OUTPUTDIR)/$@.tmp
	$(V)$(RM) -f $@
	$(V)$(CP) $(OUTPUTDIR)/$@.tmp $@
	$(V)$(RM) $(OUTPUTDIR)/$@.tmp

# TODO...
# $(OUTPUTDIR)/cfgtool_$(VERSION).exe: $(OUTPUTDIR)/cfgtool-release.exe Makefile | $(OUTPUTDIR)
# 	@echo "$(HLY)*$(HLO) $(HLC)REL$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
# 	$(V)$(CP) $< $@.tmp
# 	$(V)$(STRIP) $@.tmp
# 	$(V)$(CP) $@.tmp $@
# 	$(V)$(RM) $@.tmp

$(OUTPUTDIR)/cfgtool_$(VERSION).txt: cfgtool.txt Makefile | $(OUTPUTDIR)
	@echo "$(HLY)*$(HLO) $(HLC)REL$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(CP) $< $@

$(RELEASEZIP): $(RELEASEFILES)
	@echo "$(HLY)*$(HLO) $(HLC)ZIP$(HLO) $(HLGG)$@$(HLO) $(HLM)($(notdir $^))$(HLO)"
	$(V)$(RM) -f $@
	$(V)( cd $(OUTPUTDIR) && $(ZIP) $(notdir $@) $(notdir $^) *LICENSE* )

####################################################################################################
# Analysers

scanbuildtargets := cfgtool-release test_m32-release test_m64-release cfggui-release

.PHONY: scan-build
scan-build: $(OUTPUTDIR)/scan-build/.done

$(OUTPUTDIR)/scan-build/.done: Makefile | $(OUTPUTDIR)
	@echo "$(HLC)scan-build$(HLO) $(HLG)$(OUTPUTDIR)/scan-build$(HLO) $(HLM)($(scanbuildtargets))$(HLO)"
	$(V)$(SCANBUILD) -o $(OUTPUTDIR)/scan-build -plist-html --exclude 3rdparty $(MAKE) --no-print-directory $(scanbuildtargets) BUILDDIR=$(BUILDDIR)/scan-build
	$(V)$(TOUCH) $@

.PHONY: cfggui-valgrind
cfggui-valgrind: $(OUTPUTDIR)/cfggui-debug
	$(VALGRIND) --leak-check=full --show-leak-kinds=all --suppressions=cfggui/cfggui.supp $(OUTPUTDIR)/cfggui-debug

########################################################################################################################

LIBPREFIX := /usr/local

.PHONY: install-library
install-library:
	$(V)$(MKDIR) -p $(LIBPREFIX)/include/ubloxcfg
	$(V)$(MKDIR) -p $(LIBPREFIX)/lib/pkgconfig
	$(V)$(CP) ubloxcfg/*.h $(LIBPREFIX)/include/ubloxcfg
	$(V)$(CP) ff/*.h $(LIBPREFIX)/include/ubloxcfg
	$(V)$(SED) "s@/usr/local@$(LIBPREFIX)@" ubloxcfg/libubloxcfg.pc > $(LIBPREFIX)/lib/pkgconfig/libubloxcfg.pc
	$(V)$(CP) output/libubloxcfg.so $(LIBPREFIX)/lib

########################################################################################################################
# eof
