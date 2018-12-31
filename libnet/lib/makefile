BASE = ..

include $(BASE)/port.mak

CFLAGS = -O2 $(WARNING_FLAGS) -g -I$(INCDIR) -Iinclude -DTARGET_$(TARGET)
CXXFLAGS = $(CFLAGS)


CORE = core channels config conns lists connhelp timer fixaddr classes drivers threads wait address
DRIVERS = nonet wsockdos internet template ipx ipxsocks serial serdos serlinux local inetaddr
# Not a pleasant solution
DRIVERS += $(PLATFORM_DRIVERS)

CORE_OBJS = $(addprefix core/, $(addsuffix .o, $(CORE)))
DRIVER_OBJS = $(addprefix drivers/, $(addsuffix .o, $(DRIVERS)))


.PHONY: all lib install clean cleaner veryclean rebuild reinstall
.PRECIOUS: %/%.o


all: lib

lib: $(LIBSRC) $(INCSRC)

install: $(LIBDEST) $(INCDEST)

$(LIBDEST): $(LIBSRC)
	@echo "Copying $(LIBFILENAME) to library directory..."
	$(CP_F) $< $@

$(INCDEST): $(INCSRC)
	@echo "Copying $(INCNAME) to include directory..."
	$(CP_F) $< $@

$(LIBSRC): $(CORE_OBJS) $(DRIVER_OBJS)
	@echo "Updating library..."
	$(AR) $(ARFLAGS) $@ $^


# General dependencies
$(CORE_OBJS): $(INCSRC) include/internal.h include/drivers.h
$(DRIVER_OBJS): $(INCSRC) include/internal.h include/config.h include/platdefs.h

# Specific dependencies
core/channels.o: include/channels.h include/drivers.h
core/config.o:   include/config.h include/drivers.h
core/connhelp.o: include/config.h include/connhelp.h include/platdefs.h include/timer.h
core/conns.o:    include/conns.h include/drivers.h include/timer.h
core/core.o:     include/channels.h include/connhelp.h include/conns.h include/drivers.h
core/threads.o:  include/threads.h

drivers/ipx.o:      include/types.h
drivers/local.o:    include/types.h include/threads.h
drivers/serbeos.o:  include/serial.h
drivers/serdos.o:   include/serial.h include/serdos.h
drivers/serial.o:   include/serial.h
drivers/serlinux.o: include/serial.h
drivers/internet.o: include/inetaddr.h include/inetdefs.h
drivers/inetaddr.o: include/inetaddr.h include/inetdefs.h
drivers/wsockdos.o: include/dns.h include/inetaddr.h include/wsockdos.h

clean:
	@echo "Cleaning..."
	$(RM_F) $(CORE_OBJS) $(DRIVER_OBJS)

cleaner: clean
	@echo "Scrubbing..."
	$(RM_F) $(LIBSRC)

veryclean: cleaner
	@echo "Eradicating..."
	$(RM_F) $(LIBDEST)
	$(RM_F) $(INCDEST)


rebuild: veryclean all
reinstall: veryclean install
