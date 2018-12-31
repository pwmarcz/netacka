# Portability makefile -- helper for other makefiles

# RSXNTDJ version

TARGET = RSXNTDJ

LIBDEST = $(DJDIR)/lib/win32/$(LIBFILENAME)
INCDEST = $(DJDIR)/include/rsxntdj/$(INCNAME)
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

CC = gcc -Zwin32
WARNING_FLAGS = -Wall -Werror -Wno-unused -Wstrict-prototypes
LDLIBS = -lwsock
ARFLAGS = rs

LIBNAME = net
LIBFILENAME = lib$(LIBNAME).a
INCNAME = libnet.h
LIBDIR = $(BASE)/lib
LIBSRC = $(LIBDIR)/$(LIBFILENAME)
INCDIR = $(BASE)/include
INCSRC = $(INCDIR)/$(INCNAME)
