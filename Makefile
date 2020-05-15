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

# tools
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

# toolchain
CC             := gcc
CCWIN64        := x86_64-w64-mingw32-gcc
CCWIN32        := i686-w64-mingw32-gcc
CFLAGS         := -g -Wall -Wextra -Werror -Wpointer-arith -Wundef
TESTCFLAGS     := -std=c99 -pedantic -Wno-pedantic-ms-format
TOOLCFLAGS     := -std=gnu99
LDFLAGS        :=
LDFLAGSWIN     := -lpthread -lws2_32 -static

# source code
LIBHFILES      := $(sort $(wildcard ubloxcfg*.h))
LIBCFILES      := $(sort $(wildcard ubloxcfg*.c))
TOOLHFILES     := $(sort $(wildcard cfgtool*.h))
TOOLCFILES     := $(sort $(wildcard cfgtool*.c))
FFHFILES       := $(sort $(wildcard ff_*.h) crc24q.h)
FFCFILES       := $(sort $(wildcard ff_*.c) crc24q.c)

# output
OUTDIR         := output

####################################################################################################

.PHONY: def
def:
	@echo "Make what? Try 'make help'."

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
	@echo

# github CI, see .github/workflows/main.yml
.PHONY: gh-ci
gh-ci: test cfgtool cfgtool.txt test.exe cfgtool.exe
	$(CP) *COPYING* cfgtool.txt output/

# shortcuts
.PHONY: all
all: test cfgtool doc test.exe cfgtool.exe

.PHONY: doc
doc: $(OUTDIR)/html/index.html cfgtool.txt

.PHONY: test
test: test_m32 test_m64

.PHONY: test_m32
test_m32: $(OUTDIR)/test_m32
	$^

.PHONY: test_m64
test_m64: $(OUTDIR)/test_m64
	$^

.PHONY: test.exe
test.exe: test_m32.exe test_m64.exe

.PHONY: test_m32.exe
test_m32.exe: $(OUTDIR)/test_m32.exe

.PHONY: cfgtool
cfgtool: $(OUTDIR)/cfgtool

.PHONY: cfgtool.exe
cfgtool.exe: $(OUTDIR)/cfgtool.exe

####################################################################################################

$(OUTDIR):
	$(MKDIR) $(OUTDIR)

.PHONY: clean
clean:
ifneq ($(OUTDIR),)
	$(RM) -rf $(OUTDIR) core
endif


####################################################################################################
# config library

# generate config items info
ubloxcfg_gen.c ubloxcfg_gen.h: ubloxcfg.json ubloxcfg_gen.pl Makefile
	$(PERL) ubloxcfg_gen.pl

# documentation
$(OUTDIR)/html/index.html: $(LIBHFILES) $(LIBCFILES) Makefile Doxyfile | $(OUTDIR)
	( $(CAT) Doxyfile; $(ECHO) "OUTPUT_DIRECTORY = $(OUTDIR)"; $(ECHO) "INPUT = $(LIBHFILES) $(LIBCFILES)"; $(ECHO) "QUIET = YES"; ) | $(DOXYGEN) -

#---------------------------------------------------------------------------------------------------
# library test

$(OUTDIR)/test_m32: $(LIBCFILES) $(LIBHFILES) test.c Makefile | $(OUTDIR)
	$(CC) -m32 -std=c99 $(CFLAGS) $(TESTCFLAGS) -o $@ $(LIBCFILES) test.c

$(OUTDIR)/test_m64: $(LIBCFILES) $(LIBHFILES) test.c Makefile | $(OUTDIR)
	$(CC) -m64 -std=c99 $(CFLAGS) $(TESTCFLAGS) -o $@ $(LIBCFILES) test.c

#---------------------------------------------------------------------------------------------------
# library test (cross-compile for Windows)

$(OUTDIR)/test_m32.exe: $(LIBCFILES) $(LIBHFILES) test.c Makefile | $(OUTDIR)
	$(CCWIN32) -std=c99 $(CFLAGS) $(TESTCFLAGS) -o $@ $(LIBCFILES) test.c

.PHONY: test_m64.exe
test_m64.exe: $(OUTDIR)/test_m64.exe

$(OUTDIR)/test_m64.exe: $(LIBCFILES) $(LIBHFILES) test.c Makefile | $(OUTDIR)
	$(CCWIN64) -std=c99 $(CFLAGS) $(TESTCFLAGS) -o $@ $(LIBCFILES) test.c

####################################################################################################
# configuration tool

$(OUTDIR)/cfgtool: $(LIBHFILES) $(LIBCFILES) $(TOOLHFILES) $(TOOLCFILES) $(FFHFILES) $(FFCFILES) Makefile | $(OUTDIR)
	$(CC) $(CFLAGS) $(TOOLCFLAGS) -o $@ $(LIBCFILES) $(TOOLCFILES) $(FFCFILES) $(LDFLAGS)

cfgtool.txt: $(OUTDIR)/cfgtool
	$^ -H > $(OUTDIR)/$@.tmp
	$(RM) -f $@
	$(CP) $(OUTDIR)/$@.tmp $@
	$(RM) $(OUTDIR)/$@.tmp

#---------------------------------------------------------------------------------------------------
# cross-compile for Windows

$(OUTDIR)/cfgtool.exe: $(LIBHFILES) $(LIBCFILES) $(TOOLHFILES) $(TOOLCFILES) $(FFHFILES) $(FFCFILES) Makefile | $(OUTDIR)
	$(CCWIN64) $(CFLAGS) $(TOOLCFLAGS) -o $@ $(LIBCFILES) $(TOOLCFILES) $(FFCFILES) $(LDFLAGSWIN)

####################################################################################################
# eof
