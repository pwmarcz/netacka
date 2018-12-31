# Portability makefile -- helper for other makefiles

# MingW32 version

TARGET = MINGW32

ifdef MINGDIR
  MINGDIR_U = $(subst \,/,$(MINGDIR))
  MINGDIR_D = $(subst /,\,$(MINGDIR))
else
	@echo Please specify your mingw32 directory via MINGDIR=.
endif

LIBDEST = $(MINGDIR_U)/lib/$(LIBFILENAME)
INCDEST = $(MINGDIR_U)/include/$(INCNAME)
EXE_SUFFIX = .exe

# Fileutils is required
RM_F = rm -f
CP_F = cp

# DOSish analogue of the "%: %.o" rule
%.exe: %.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

CC = gcc
WARNING_FLAGS = -Wall -Werror -Wno-unused -Wstrict-prototypes
LDLIBS = -lwsock32
LINK_ALLEGRO = -lalleg
ARFLAGS = rs

LIBNAME = net
LIBFILENAME = lib$(LIBNAME).a
INCNAME = libnet.h
LIBDIR = $(BASE)/lib
LIBSRC = $(LIBDIR)/$(LIBFILENAME)
INCDIR = $(BASE)/include
INCSRC = $(INCDIR)/$(INCNAME)
