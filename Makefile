########################################################################################################################
#
# u-blox 9 positioning receivers configuration library and tool
#
# Copyright (c) 2020 Philippe Kehl (flipflip at oinkzwurgl dot org),
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

# ff library souces
CFILES_ff             := $(wildcard ff/*.c)
CXXFILES_ff           := $(wildcard ff/*.cpp)

# Toolchain flags
CFLAGS_all            := -Wall -Wextra -Werror -Wshadow
CXXFLAGS_all          := -Wall -Wextra -Werror -Wshadow
LDFLAGS_all           :=

CFLAGS_release        := -DFF_BUILD_RELEASE -O3 -g
CXXFLAGS_release      := -DFF_BUILD_RELEASE -O3 -g
LDFLAGS_release       := -g

CFLAGS_debug          := -DFF_BUILD_DEBUG -Og -ggdb
CXXFLAGS_debug        := -DFF_BUILD_DEBUG -Og -ggdb
LDFLAGS_debug         := -ggdb

CFLAGS_gperf          := -DFF_BUILD_GPERF -Og -ggdb
CXXFLAGS_gperf        := -DFF_BUILD_GPERF -Og -ggdb
LDFLAGS_gperf         := -ggdb -lprofiler

# test
CFILES_test_m32       := test.c
CFLAGS_test_m32       := -std=c99 -pedantic -Wno-pedantic-ms-format -m32
LDFLAGS_test_m32      := -m32
CFILES_test_m64       := test.c
CFLAGS_test_m64       := -std=c99 -pedantic -Wno-pedantic-ms-format -m64
LDFLAGS_test_m64      := -m64

# cfgtool
CFILES_cfgtool        := cfgtool.c $(wildcard cfgtool/*.c) 3rdparty/stuff/crc24q.c
CFLAGS_cfgtool        := -std=gnu99 -Wformat -Wpointer-arith -Wundef
LDFLAGS_cfgtool       := -lm
$(CFILES_cfgtool): $(BUILDDIR)/config.h
ifeq ($(WIN),64)
LDFLAGS_cfgtool       += -lws2_32 -static
endif

# Binaries, makeTarget: name, .c/.cpp files, CFLAGS, CXXFLAGS, LDFLAGS -- The final CFLAGS, CXXFLAGS and LDFLAGS will be $(CFLAGS) etc. from the environment, + those given here
$(eval $(call makeTarget, test_m32-release$(EXE), $(CFILES_test_m32) $(CFILES_ubloxcfg),                                 $(CFLAGS_all) $(CFLAGS_release) $(CFLAGS_test_m32),                                                       , $(LDLFAGS_all) $(LDFLAGS_release) $(LDFLAGS_test_m32)))
$(eval $(call makeTarget, test_m32-debug$(EXE),   $(CFILES_test_m32) $(CFILES_ubloxcfg),                                 $(CFLAGS_all) $(CFLAGS_debug)   $(CFLAGS_test_m32),                                                       , $(LDLFAGS_all) $(LDFLAGS_debug)   $(LDFLAGS_test_m32)))
$(eval $(call makeTarget, test_m32-gperf$(EXE),   $(CFILES_test_m32) $(CFILES_ubloxcfg),                                 $(CFLAGS_all) $(CFLAGS_gperf)   $(CFLAGS_test_m32),                                                       , $(LDLFAGS_all) $(LDFLAGS_gperf)   $(LDFLAGS_test_m32)))
$(eval $(call makeTarget, test_m64-release$(EXE), $(CFILES_test_m64) $(CFILES_ubloxcfg),                                 $(CFLAGS_all) $(CFLAGS_release) $(CFLAGS_test_m64),                                                       , $(LDLFAGS_all) $(LDFLAGS_release) $(LDFLAGS_test_m64)))
$(eval $(call makeTarget, test_m64-debug$(EXE),   $(CFILES_test_m64) $(CFILES_ubloxcfg),                                 $(CFLAGS_all) $(CFLAGS_debug)   $(CFLAGS_test_m64),                                                       , $(LDLFAGS_all) $(LDFLAGS_debug)   $(LDFLAGS_test_m64)))
$(eval $(call makeTarget, test_m64-gperf$(EXE),   $(CFILES_test_m64) $(CFILES_ubloxcfg),                                 $(CFLAGS_all) $(CFLAGS_gperf)   $(CFLAGS_test_m64),                                                       , $(LDLFAGS_all) $(LDFLAGS_gperf)   $(LDFLAGS_test_m64)))
$(eval $(call makeTarget, cfgtool-release$(EXE),  $(CFILES_cfgtool)  $(CFILES_ubloxcfg) $(CFILES_ff) $(CFILES_cfgtool),  $(CFLAGS_all) $(CFLAGS_release) $(CFLAGS_cfgtool),                                                        , $(LDLFAGS_all) $(LDFLAGS_release) $(LDFLAGS_cfgtool)))
$(eval $(call makeTarget, cfgtool-debug$(EXE),    $(CFILES_cfgtool)  $(CFILES_ubloxcfg) $(CFILES_ff) $(CFILES_cfgtool),  $(CFLAGS_all) $(CFLAGS_debug)   $(CFLAGS_cfgtool),                                                        , $(LDLFAGS_all) $(LDFLAGS_debug)   $(LDFLAGS_cfgtool)))
$(eval $(call makeTarget, cfgtool-gperf$(EXE),    $(CFILES_cfgtool)  $(CFILES_ubloxcfg) $(CFILES_ff) $(CFILES_cfgtool),  $(CFLAGS_all) $(CFLAGS_gperf)   $(CFLAGS_cfgtool),                                                        , $(LDLFAGS_all) $(LDFLAGS_gperf)   $(LDFLAGS_cfgtool)))

########################################################################################################################

# Make config.h
$(BUILDDIR)/config.h: config.h.in config.h.pl Makefile | $(BUILDDIR)
	@echo "$(HLY)*$(HLO) $(HLC)GEN$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(PERL) config.h.pl < config.h.in > $@.tmp
	$(V)$(CP) $@.tmp $@
	$(V)$(RM) $@.tmp

# Generate config items info
ubloxcfg/ubloxcfg_gen.c ubloxcfg/ubloxcfg_gen.h: ubloxcfg/ubloxcfg.json ubloxcfg/ubloxcfg_gen.pl Makefile
	@echo "$(HLY)*$(HLO) $(HLC)GEN$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(PERL) ubloxcfg/ubloxcfg_gen.pl

ubloxcfg/ubloxcfg.c: ubloxcfg/ubloxcfg_gen.c ubloxcfg/ubloxcfg_gen.h

# Documentation
$(OUTPUTDIR)/ubloxcfg_html/index.html: ubloxcfg/Doxyfile $(LIBHFILES) $(LIBCFILES) Makefile | $(OUTPUTDIR)
	@echo "$(HLY)*$(HLO) $(HLC)doxygen$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)( $(CAT) $<; $(ECHO) "OUTPUT_DIRECTORY = $(OUTPUTDIR)"; $(ECHO) "INPUT = $(LIBHFILES) $(LIBCFILES)"; $(ECHO) "QUIET = YES"; ) | $(DOXYGEN) -

########################################################################################################################

# Used in .github/workflows/main.yml
.PHONY: ci
ci: all

# Make everything
.PHONY: all
all: test_m32-release test_m64-release cfgtool-release release

# Some shortcuts
test_m32: test_m32-release
test_m64: test_m64-release
test: test_m32 test_m64
	$(OUTPUTDIR)/test_m32-release
	$(OUTPUTDIR)/test_m64-release
.PHONY: cfgtool
cfgtool: cfgtool-release
.PHONY: doc
doc: $(OUTPUTDIR)/ubloxcfg_html/index.html cfgtool.txt

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
	@echo "    <prog>-<build>  Make binary, <prog> is cfgtool, ... and <build> is release, debug, or gperf"
	@echo "    test            Build and run tests"
	@echo "    doc             Build HTML docu of the ubloxcfg libaray"
	@echo "    debugmf         Show some Makefile variables"
	@echo "    scan-build      Run scan-build"
	@echo
	@echo "To cross-compile for windows, use 'make <prog>.exe-<build> WIN=64'."
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
# Release

LICENSES_IN := $(wildcard *COPYING*) $(wildcard 3rdparty/stuff/*COPYING*)
LICENSES_OUT := $(addprefix $(OUTPUTDIR)/, $(notdir $(LICENSES_IN)))

RELEASEFILES := $(sort $(OUTPUTDIR)/cfgtool_$(VERSION).bin  \
    $(OUTPUTDIR)/cfgtool_$(VERSION).txt $(LICENSES_OUT))
# TODO $(OUTPUTDIR)/cfgtool_$(VERSION).exe

RELEASEZIP := $(OUTPUTDIR)/ubloxcfg_$(VERSION).zip

.PHONY: release
release: $(RELEASEZIP)

$(OUTPUTDIR)/cfgtool_$(VERSION).bin: $(OUTPUTDIR)/cfgtool-release Makefile | $(OUTPUTDIR)
	@echo "$(HLY)*$(HLO) $(HLC)REL$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(CP) $< $@.tmp
	$(V)$(STRIP) $@.tmp
	$(V)$(CP) $@.tmp $@
	$(V)$(RM) $@.tmp

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

$(LICENSES_OUT): $(LICENSES_IN) Makefile | $(OUTPUTDIR)
	@echo "$(HLY)*$(HLO) $(HLC)CP$(HLO) $(HLG)$(LICENSES_IN)$(HLO)"
	$(V)$(CP) $(LICENSES_IN) $(OUTPUTDIR)

$(RELEASEZIP): $(RELEASEFILES)
	@echo "$(HLY)*$(HLO) $(HLC)ZIP$(HLO) $(HLGG)$@$(HLO) $(HLM)($(notdir $^))$(HLO)"
	$(V)$(RM) -f $@
	$(V)( cd $(OUTPUTDIR) && $(ZIP) $(notdir $@) $(notdir $^) )

####################################################################################################
# Analysers

scanbuildtargets := cfgtool-release test_m32-release test_m64-release

.PHONY: scan-build
scan-build: $(OUTPUTDIR)/scan-build/.done

$(OUTPUTDIR)/scan-build/.done: Makefile | $(OUTPUTDIR)
	@echo "$(HLC)scan-build$(HLO) $(HLG)$(OUTPUTDIR)/scan-build$(HLO) $(HLM)($(scanbuildtargets))$(HLO)"
	$(V)$(SCANBUILD) -o $(OUTPUTDIR)/scan-build -plist-html --exclude 3rdparty $(MAKE) --no-print-directory $(scanbuildtargets) BUILDDIR=$(BUILDDIR)/scan-build
	$(V)$(TOUCH) $@

########################################################################################################################
# eof
