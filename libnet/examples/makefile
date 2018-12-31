BASE = ..

include $(BASE)/port.mak

CFLAGS = -O2 -Wall -Werror -Wno-unused -g -I$(INCDIR) -DTARGET_$(TARGET)

LINUX_EXAMPLES   = chat/client chat/server rdmchat/client rdmchat/server
DJGPP_EXAMPLES   = chat/client chat/server rdmchat/client rdmchat/server
FREEBSD_EXAMPLES =
RSXNTDJ_EXAMPLES =
BEOS_EXAMPLES    = 

ifeq ($(TARGET),LINUX)
LDLIBS += -lncurses
endif

ifneq ($(USE_ALLEGRO),)
LDLIBS += $(LINK_ALLEGRO)
CFLAGS += -DTARGET_ALLEGRO
endif


EXAMPLES += $($(TARGET)_EXAMPLES)


EXES = $(addsuffix $(EXE_SUFFIX), $(EXAMPLES))
OBJS = $(addsuffix .o, $(EXAMPLES))


.PHONY: all clean cleaner veryclean

all: $(EXES)

$(EXES): $(LIBSRC)
$(OBJS): $(INCSRC)

clean:
	$(RM_F) $(OBJS)

cleaner: clean
	$(RM_F) $(EXES) */core

veryclean: cleaner

