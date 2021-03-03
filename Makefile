# -*- Mode: makefile -*-
#
# This Makefile example is fairly independent from the main makefile
# so users can take and adapt it for their build. We only really
# include config-host.mak so we don't have to repeat probing for
# cflags that the main configure has already done for us.
#

QEMU_BUILD_DIR := $(CURDIR)/./qemu/bin
include $(QEMU_BUILD_DIR)/config-host.mak

# The main QEMU uses Glib extensively so it's perfectly fine to use it
# in plugins (which many example do).
CFLAGS = $(GLIB_CFLAGS) $(LIBXML2_CFLAGS) -fPIC
CFLAGS += $(if $(findstring no-psabi,$(QEMU_CFLAGS)),-Wpsabi)
CFLAGS += -I$(SRC_PATH)/include/qemu -Wall -Werror

OS := $(shell uname)
ifeq ($(OS), Darwin)
LDFLAGS = -shared -undefined dynamic_lookup -Wl,-install_name,$@
else ifeq ($(OS), Linux)
LDFLAGS = -shared -Wl,-soname,$@
else
$(error Unsupported OS, cannot build shared library for plugin)
endif

all: tests

libqta.so: src/plugin.c src/qta.c
	@echo -e '--------------------------------------------------------------------------------'
	@echo -e '-  Compile libqta.so                                                           -'
	@echo -e '--------------------------------------------------------------------------------'
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBXML2_LIBS)

tests: libqta.so
	$(MAKE) -s -C tutorial all

clean:
	rm -f libqta.so
	$(MAKE) -s -C tutorial clean

.PHONY: all tests clean
