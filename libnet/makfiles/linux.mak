# Portability makefile -- helper for other makefiles

# Linux version

TARGET = LINUX

LIBDEST = /usr/local/lib/$(LIBFILENAME)
INCDEST = /usr/local/include/$(INCNAME)
EXE_SUFFIX =
RM_F = rm -f
CP_F = cp -f

CC = gcc
WARNING_FLAGS = -Wall -Wno-unused -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
LDLIBS = -lpthread
LINK_ALLEGRO = `allegro-config --libs`
ARFLAGS = rs

LIBNAME = net
LIBFILENAME = lib$(LIBNAME).a
INCNAME = libnet.h
LIBDIR = $(BASE)/lib
LIBSRC = $(LIBDIR)/$(LIBFILENAME)
INCDIR = $(BASE)/include
INCSRC = $(INCDIR)/$(INCNAME)
