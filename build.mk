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

########################################################################################################################
# Tools, toolchain, other variables

# Tools
PERL           := perl
TOUCH          := touch
CP             := cp
MKDIR          := mkdir
TOUCH          := touch
ECHO           := echo
CAT            := cat
RM             := rm
MV             := mv
TRUE           := true
FALSE          := false
DOXYGEN        := doxygen
SED            := sed
ZIP            := zip
VALGRIND       := valgrind
XXD            := xxd

# Toolchain (prefer environment variables over the defaults here; ignore Make's defaults for CC and LD)
ifeq ($(origin CC),default)
CC              = gcc
endif
ifeq ($(origin CXX),default)
CXX             = gcc
endif
ifeq ($(origin LD),default)
LD              = gcc
endif
NM             ?= nm
OBJCOPY        ?= objcopy
OBJDUMP        ?= objdump
RANLIB         ?= ranlib
READELF        ?= readelf
STRINGS        ?= strings
STRIP          ?= strip
CFLAGS         ?=
CXXFLAGS       ?=
LDFLAGS        ?=
SCANBUILD      ?= scan-build

# Verbosity helpers
ifeq ($(VERBOSE),1)
V =
V1 =
V2 =
V12 =
RM += -v
MV += -v
CP += -v
MKDIR += -v
else
ZIP += -q
UNZIP += -q
V = @
V1 = > /dev/null
V2 = 2> /dev/null
V12 = 2>&1 > /dev/null
endif

# Colours!
fancyterm := true
ifeq ($(TERM),dumb)
fancyterm := false
endif
ifeq ($(TERM),)
fancyterm := false
endif
ifneq ($(MSYSTEM),)
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

########################################################################################################################
# Canned recipes to generate compile and link rules

make_deps := Makefile $(sort $(wildcard *.mk))

# Make rules for one target
define makeTarget
  # 1) prog-build target name: "rtknav-release", "str2str-debug", ...
  # 2) list of .c and .cpp files for the program
  # 3) CFLAGS
  # 4) CXXFLAGS
  # 5) LDFLAGS

  # $ (info makeTarget: [$(strip $1)] [$2])

  # Make rules for compiling each source file, add to list of object files
  OFILES_$(strip $1) :=
  $$(foreach src, $$(filter %.c, $$(sort $2)), $$(eval $$(call makeCompileRuleC, $1, $$(src), $(BUILDDIR)/$(strip $1))))
  $$(foreach src, $$(filter %.cpp, $$(sort $2)), $$(eval $$(call makeCompileRuleCxx, $1, $$(src), $(BUILDDIR)/$(strip $1))))

  # Build (object) directory for this target (e.g. "build/rtknav-release")
  $(BUILDDIR)/$(strip $1): | $(BUILDDIR)
	$(V)$(MKDIR) $$@

  # CFLAGS and CXXFLAGS for this target, add -I flags for this target for all directories
  CFLAGS_$(strip $1)   := $3 $(addprefix -I,$(sort $(dir $2))) -I$(BUILDDIR)
  CXXFLAGS_$(strip $1) := $4 $(addprefix -I,$(sort $(dir $2))) -I$(BUILDDIR)

  # LDFLAGS for this target
  LDFLAGS_$(strip $1) := $5

  # Link objects into executable
  $(OUTPUTDIR)/$(strip $1): $$(OFILES_$(strip $1)) | $(OUTPUTDIR)
	@echo "$(HLY)*$(HLO) $(HLC)LD$(HLO) $(HLGG)$$@$(HLO)"
	$(V)$(CC) -o $$@ $$(OFILES_$(strip $1)) $(LDFLAGS) $$(LDFLAGS_$(strip $1))

  # Create pure binary (output/foo-release -> build/foo-release/foo-release.bin)
  $(BUILDDIR)/$(strip $1)/$(strip $1).bin: $(OUTPUTDIR)/$(strip $1)
	@echo "$(HLY)*$(HLO) $(HLC)OBJCOPY$(HLO) $(HLG)$$@$(HLO) $(HLM)($$<)$(HLO)"
	$(V)$(RM) -f $$@
	$(V)$(OBJCOPY) $$< -O binary $$@.tmp
	$(V)$(MV) $$@.tmp $$@

  # Create listing (output/foo-release -> build/foo-release/foo-release.lst)
  # $(BUILDDIR)/$(strip $1)/$(strip $1).lst: $(OUTPUTDIR)/$(strip $1)
  #	@echo "$(HLY)*$(HLO) $(HLC)OBJDUMP$(HLO) $(HLG)$$@$(HLO) $(HLM)($$<)$(HLO)"
  #	$(V)$(RM) -f $$@
  #	$(V)$(OBJDUMP) -d -S $$< > $$@.tmp
  #	$(V)$(MV) $$@.tmp $$@

  # Create dump of all sections (output/foo-release -> build/foo-release/foo-release.dump)
  $(BUILDDIR)/$(strip $1)/$(strip $1).dump: $(OUTPUTDIR)/$(strip $1)
	@echo "$(HLY)*$(HLO) $(HLC)READELF$(HLO) $(HLG)$$@$(HLO) $(HLM)($$<)$(HLO)"
	$(V)$(RM) -f $$@
	$(V)$(READELF) --wide --section-headers $$< >> $$@.tmp
	$(V)$(READELF) --wide --file-header     $$< >> $$@.tmp
	$(V)for section in $$$$($(PERL) -ne 'if (/^\s*\[\s*\d+\s*\]\s+(\S+)/ && ($$$$1 !~ m{^(NULL|\.debug_.+)$$$$})) { print "$$$$1\n"; }' < $$@.tmp); do \
	    $(READELF) --wide --hex-dump="$$$$section" $$<; \
	done >> $$@.tmp
	$(V)$(MV) $$@.tmp $$@

  # Create dump of preprocessor defines
  $(BUILDDIR)/$(strip $1)/$(strip $1).defines: $(make_deps) | $(BUILDDIR)/$(strip $1)
	@echo "$(HLY)*$(HLO) $(HLC)GEN$(HLO) $(HLG)$$@$(HLO) $(HLM)($(CC))$(HLO)"
	$(V)$(RM) -f $$@ $$@.tmp
	$(V)$(ECHO) "// $(CC) $(CFLAGS) $$(CFLAGS_$(strip $1))" >> $$@.tmp
	$(V)$(ECHO) | $(CC) $(CFLAGS) $$(CFLAGS_$(strip $1)) -dM -E - >> $$@.tmp
	$(V)$(MV) $$@.tmp $$@

  # Shortcut for the binary (rtknav-debug = output/rtknav-debug)
  .PHONY: $(strip $1)
  $(strip $1): $(OUTPUTDIR)/$(strip $1) $(foreach ext, bin dump defines, $(BUILDDIR)/$(strip $1)/$(strip $1).$(ext))

endef

# Make rule to compile single .c file
define makeCompileRuleC
  # 1) app-build target name
  # 2) .c file
  # 3) build dir

  # $ (info makeCompileRuleC [$(strip $1)] [$(strip $2)] [$(strip $3)])

  # Compile src/foo/bar.c --> build/prog-build/src__foo__bar.o
  $(strip $3)/$(subst /,__,$(patsubst %.c,%.o,$2)): $2 $(make_deps) | $3
	@echo "$(HLY)*$(HLO) $(HLC)CC$(HLO) $(HLG)$$<$(HLO) $(HLM)($$@)$(HLO)"
	$(V)$(CC) -c -o $$@ $(CFLAGS) $$(CFLAGS_$(strip $1)) $$< -MD -MF $$(@:%.o=%.d) -MT $$@

  # Add to list of object files for this target
  OFILES_$(strip $1) += $(strip $3)/$(subst /,__,$(patsubst %.c,%.o,$2))

endef

# Make rule to compile single .cpp file
define makeCompileRuleCxx
  # 1) app-build target name
  # 2) .cpp file
  # 3) build dir

  # $ (info makeCompileRuleCxx [$(strip $1)] [$(strip $2)] [$(strip $3)])

  # Compile src/foo/bar.cpp --> build/prog-build/src__foo__bar.o
  $(strip $3)/$(subst /,__,$(patsubst %.cpp,%.o,$2)): $2 $(make_deps) | $3
	@echo "$(HLY)*$(HLO) $(HLC)CXX$(HLO) $(HLG)$$<$(HLO) $(HLM)($$@)$(HLO)"
	$(V)$(CXX) -c -o $$@ $(CXXFLAGS) $$(CXXFLAGS_$(strip $1)) $$< -MD -MF $$(@:%.o=%.d) -MT $$@

  # Add to list of object files for this target
  OFILES_$(strip $1) += $(strip $3)/$(subst /,__,$(patsubst %.cpp,%.o,$2))

endef
########################################################################################################################
# Generated version

$(BUILDDIR)/std_version_gen.h: util/build/std_version_gen.h.pl $(make_deps) | $(BUILDDIR)
	@echo "$(HLY)*$(HLO) $(HLC)GEN$(HLO) $(HLG)$@$(HLO) $(HLM)($<)$(HLO)"
	$(V)$(RM) -f $@ $@.tmp
	$(V)$(PERL) $< >$@.tmp
	$(V)$(MV) $@.tmp $@

src/std/std_version.c: $(BUILDDIR)/std_version_gen.h

########################################################################################################################
# Load dependency files

ifneq ($(MAKECMDGOALS),clean)
-include $(sort $(wildcard $(BUILDDIR)/*/*.d))
endif

########################################################################################################################
# Auxiliary targets

$(BUILDDIR):
	$(V)$(MKDIR) $@

$(OUTPUTDIR):
	$(V)$(MKDIR) $@

.PHONY: clean
clean:
	$(V)$(RM) -rf $(BUILDDIR) $(OUTPUTDIR) core core.*

.PHONY: debugmf
debugmf:
	@echo "OUTPUTDIR = $(OUTPUTDIR) ($(origin OUTPUTDIR))"
	@echo "BUILDDIR  = $(BUILDDIR)  ($(origin BUILDDIR))"
	@echo "CC        = $(CC) ($(origin CC))"
	@echo "CXX       = $(CXX) ($(origin CXX))"
	@echo "LD        = $(LD) ($(origin LD))"
	@echo "NM        = $(NM) ($(origin NM))"
	@echo "OBJCOPY   = $(OBJCOPY) ($(origin OBJCOPY))"
	@echo "OBJDUMP   = $(OBJDUMP) ($(origin OBJDUMP))"
	@echo "RANLIB    = $(RANLIB) ($(origin RANLIB))"
	@echo "READELF   = $(READELF) ($(origin READELF))"
	@echo "STRINGS   = $(STRINGS) ($(origin STRINGS))"
	@echo "STRIP     = $(STRIP) ($(origin STRIP))"
	@echo "CFLAGS    = $(CFLAGS) ($(origin CFLAGS))"
	@echo "CXXFLAGS  = $(CXXFLAGS) ($(origin CXXFLAGS))"
	@echo "LDFLAGS   = $(LDFLAGS) ($(origin LDFLAGS))"
	@echo "SCANBUILD = $(SCANBUILD) ($(origin SCANBUILD))"

########################################################################################################################
# eof
