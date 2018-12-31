# Portability makefile -- helper for other makefiles

# BeOS version

TARGET = BEOS

# PW: I'm pretty sure these are incorrect.
LIBDEST = /boot/develop/lib/x86/$(LIBFILENAME)
INCDEST = /boot/develop/headers/$(INCNAME)
EXE_SUFFIX =
LDLIBS = -ldevice
RM_F = rm -f
CP_F = cp -f

CC = gcc
WARNING_FLAGS = -Wall -Werror -Wno-unused -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
ARFLAGS = rs

LIBNAME = libnet
LIBFILENAME = lib$(LIBNAME).a
INCNAME = libnet.h
LIBDIR = $(BASE)/lib
LIBSRC = $(LIBDIR)/$(LIBFILENAME)
INCDIR = $(BASE)/include
INCSRC = $(INCDIR)/$(INCNAME)

PLATFORM_DRIVERS = serbeos

