########################################################################################################################

.PHONY: default
default:
	@echo "Make what? Try 'make help'!"
	@exit 1

# Defaults (keep in sync with help screen below)
BUILD_TYPE     = Debug
INSTALL_PREFIX = install
BUILD_TESTING  =
VERBOSE        = 0

# User vars
-include config.mk

# A unique ID for this exact config we're using
configuid=$(shell echo "$(BUILD_TYPE) $(INSTALL_PREFIX) $(BUILD_TESTING) ${FF_VERSION_STRING} $$(uname -a)" | md5sum | cut -d " " -f1)

.PHONY: help
help:
	@echo "Usage:"
	@echo
	@echo "    make <target> [INSTALL_PREFIX=...] [BUILD_TYPE=Debug|Release] [BUILD_TESTING=|ON|OFF] [FF_VERSION_STRING=x.x.x-gggggggg] [VERBOSE=1]"
	@echo
	@echo "Where possible <target>s are:"
	@echo
	@echo "    clean               Clean build directory"
	@echo "    cmake               Configure"
	@echo "    build               Build"
	@echo "    test                Run tests"
	@echo "    install             Install (into INSTALL_PREFIX path)"
	@echo "    doc                 Generate documentation (into build directory)"
	@echo "    doc-dev             Generate documentation and start webserver to view it"
	@echo
	@echo "Typically you want to do something like this:"
	@echo
	@echo "     make install INSTALL_PREFIX=~/ubloxcfg"
	@echo
	@echo "Notes:"
	@echo
	@echo "- All <target>s but 'clean' require that the same command-line variables are passed"
	@echo "- Command-line variables can be stored into a config.mk file, which is automatically loaded"
	@echo "- 'make ci' runs the CI (more or less) like on Github. INSTALL_PREFIX and BUILD_TYPE have no effect here."
	@echo

########################################################################################################################

TOUCH      := touch
MKDIR      := mkdir
ECHO       := echo
RM         := rm
CMAKE      := cmake
DOXYGEN    := doxygen
NICE       := nice
CAT        := cat
PYTHON     := python
SED        := sed
LN         := ln

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
HLW="\\e[1m"
HLO="\\e[m"
else
HLW=
HLO=
endif

# Disable all built-in rules
.SUFFIXES:

########################################################################################################################

CMAKE_ARGS_BUILD :=
CMAKE_ARGS_INSTALL :=
CMAKE_ARGS := -DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX)
CMAKE_ARGS += -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
ifneq ($(BUILD_TESTING),)
  CMAKE_ARGS += -DBUILD_TESTING=$(BUILD_TESTING)
endif
ifneq ($(FF_VERSION_STRING),)
  CMAKE_ARGS += -DVERSION_STRING=$(FF_VERSION_STRING)
endif

ifeq ($(BUILD_TYPE),Release)
  CMAKE_ARGS_INSTALL += --strip
endif

MAKEFLAGS = --no-print-directory

ifneq ($(VERBOSE),0)
  CMAKE_ARGS_BUILD += --verbose
endif

ifeq ($(GITHUB_WORKSPACE),)
  CMAKE_ARGS_BUILD = --parallel $(shell nproc --ignore=2)
  NICE_BUILD=$(NICE) -19
else
  CMAKE_ARGS_BUILD = --parallel 4
  NICE_BUILD=
endif

BUILD_DIR = build/$(BUILD_TYPE)

# "All-in-one" targets
.PHONY: clean
clean:
	$(V)$(RM) -rf $(BUILD_DIR)

.PHONY: distclean
distclean:
	$(V)$(RM) -rf install build core.[123456789]*

$(BUILD_DIR):
	$(V)$(MKDIR) -p $@

# Detect changed build config
ifneq ($(MAKECMDGOALS),)
ifneq ($(MAKECMDGOALS),help)
ifneq ($(MAKECMDGOALS),pre-commit)
ifneq ($(MAKECMDGOALS),ci)
ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(MAKECMDGOALS),doc)
builddiruid=$(shell $(CAT) $(BUILD_DIR)/.make-uid 2>/dev/null || echo "none")
ifneq ($(builddiruid),$(configuid))
    dummy=$(shell $(RM) -vf $(BUILD_DIR)/.make-uid))
endif
endif
endif
endif
endif
endif
endif

$(BUILD_DIR)/.make-uid: | $(BUILD_DIR)
	$(V)$(ECHO) $(configuid) > $@

# ----------------------------------------------------------------------------------------------------------------------

.PHONY: cmake
cmake: $(BUILD_DIR)/.make-cmake

deps_cmake := Makefile $(wildcard config.mk) $(sort $(wildcard CMakeLists.txt */CMakeLists.txt cmake/*))

$(BUILD_DIR)/.make-cmake: $(deps_cmake) $(BUILD_DIR)/.make-uid
	@echo "$(HLW)***** Configure ($(BUILD_TYPE)) *****$(HLO)"
	$(V)$(CMAKE) -B $(BUILD_DIR) $(CMAKE_ARGS)
	$(V)$(TOUCH) $@

# ----------------------------------------------------------------------------------------------------------------------

.PHONY: build
build: $(BUILD_DIR)/.make-build

deps_build = $(sort $(wildcard ubloxcfg/* ffxx/* ffapps/* ffapps/*/* cfgtool/*))

$(BUILD_DIR)/.make-build: $(deps_build) $(BUILD_DIR)/.make-cmake $(BUILD_DIR)/.make-uid
	@echo "$(HLW)***** Build ($(BUILD_TYPE)) *****$(HLO)"
	$(V)$(NICE_BUILD) $(CMAKE) --build $(BUILD_DIR) $(CMAKE_ARGS_BUILD) -- -k
	$(V)$(TOUCH) $@

# ----------------------------------------------------------------------------------------------------------------------

.PHONY: install
install: $(BUILD_DIR)/.make-install

$(BUILD_DIR)/.make-install: $(BUILD_DIR)/.make-build
	@echo "$(HLW)***** Install ($(BUILD_TYPE)) *****$(HLO)"
	$(V)$(CMAKE) --install $(BUILD_DIR) $(CMAKE_ARGS_INSTALL)
	$(V)$(TOUCH) $@

# ----------------------------------------------------------------------------------------------------------------------

.PHONY: test
test: $(BUILD_DIR)/.make-build
	@echo "$(HLW)***** Test ($(BUILD_TYPE)) *****$(HLO)"
ifeq ($(VERBOSE),1)
	$(V)$(BUILD_DIR)/ubloxcfg/ubloxcfg-test -v
else
	$(V)$(BUILD_DIR)/ubloxcfg/ubloxcfg-test
endif

# ----------------------------------------------------------------------------------------------------------------------

.PHONY: doc
doc: $(BUILD_DIR)/.make-doc
	@echo "now run: xdg-open $(BUILD_DIR)/ubloxcfg/doc/index.html"

$(BUILD_DIR)/.make-doc: $(BUILD_DIR)/.make-build ubloxcfg/Doxyfile
	@echo "$(HLW)***** Doc ($(BUILD_TYPE)) *****$(HLO)"
	$(V)( \
            cat ubloxcfg/Doxyfile; \
            echo "PROJECT_NUMBER = $$(cat $(BUILD_DIR)/FF_VERSION_STRING || echo 'unknown revision')"; \
            echo "OUTPUT_DIRECTORY = $(BUILD_DIR)/ubloxcfg"; \
			echo "INPUT = ubloxcfg"; \
        ) | $(DOXYGEN) -
	$(V)$(TOUCH) $@

.PHONY: doc-dev
doc-dev: $(BUILD_DIR)/.make-doc
	$(V)(cd $(BUILD_DIR)/ubloxcfg/doc && $(PYTHON) -m http.server 8000)

# ----------------------------------------------------------------------------------------------------------------------

# .PHONY: ci
# ci: $(BUILD_DIR)/.ci-bookworm $(BUILD_DIR)/.ci-noetic $(BUILD_DIR)/.ci-humble $(BUILD_DIR)/.ci-jazzy

# $(BUILD_DIR)/.ci-bookworm: $(deps)
# 	@echo "$(HLW)***** CI (bookworm) *****$(HLO)"
# ifeq ($(FPSDK_IMAGE),)
# 	$(V)docker/docker.sh run bookworm-ci ./docker/ci.sh
# 	$(V)$(TOUCH) $@
# else
# 	@echo "This ($@) should not run inside Docker!"
# 	@false
# endif

# ----------------------------------------------------------------------------------------------------------------------

# .PHONY: pre-commit
# pre-commit: | $(BUILD_DIR)
# 	@echo "$(HLW)***** pre-commit checks *****$(HLO)"
# 	$(V)$(MKDIR) -p $(BUILD_DIR)/.bin
# 	$(V)$(LN) -sf /usr/bin/clang-format-17 $(BUILD_DIR)/.bin/clang-format
# 	$(V)export PATH=$(BUILD_DIR)/.bin:$$PATH; pre-commit run --all-files --hook-stage manual || return 1

########################################################################################################################
