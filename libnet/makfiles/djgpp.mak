# Portability makefile -- helper for other makefiles

# DJGPP version

TARGET = DJGPP

LIBDEST = $(DJDIR)/lib/$(LIBFILENAME)
INCDEST = $(DJDIR)/include/$(INCNAME)
EXE_SUFFIX = .exe
ifneq ($(wildcard $(DJDIR)/bin/rm.exe),)
  RM_F = rm -f
else           # Argh!  No fileutils!  Now what?
  RM_F = @echo You don\'t have fileutils.  Remove the following files by hand:
endif

# `update' is a djgpp-supplied file copier
CP_F = update

# DOSish analogue of the "%: %.o" rule
%.exe: %.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

CC = gcc
WARNING_FLAGS = -Wall -Werror -Wno-unused -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
ARFLAGS = rs
LINK_ALLEGRO = -lalleg

LIBNAME = net
LIBFILENAME = lib$(LIBNAME).a
INCNAME = libnet.h
LIBDIR = $(BASE)/lib
LIBSRC = $(LIBDIR)/$(LIBFILENAME)
INCDIR = $(BASE)/include
INCSRC = $(INCDIR)/$(INCNAME)
