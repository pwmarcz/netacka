BASE = ..

include $(BASE)/port.mak

CFLAGS = -O2 -Wall -Werror -Wno-unused -g -I$(INCDIR) -DTARGET_$(TARGET)


TESTS = getdrvnm gentest rdmtest fixaddr classes dnstest

EXES = $(addsuffix $(EXE_SUFFIX), $(TESTS))
OBJS = $(addsuffix .o, $(TESTS))


.PHONY: all clean cleaner veryclean

all: $(EXES)

$(EXES): $(LIBSRC)
$(OBJS): $(INCSRC)

clean:
	$(RM_F) $(OBJS)

cleaner: clean
	$(RM_F) $(EXES) core

veryclean: cleaner
