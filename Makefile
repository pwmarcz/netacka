
.PHONY = all clean

BOTS = bot.o genialny_bot0.o genialny_bot.o
OBJECTS_EXE = netacka.o net.o ui.c gui/gui.o $(BOTS)

CC = gcc
CFLAGS = -s -O2 -Wall -fgnu89-inline -Ilibnet/include
#CFLAGS = -g3 -fgnu89-inline

# change these for other systems
LIB = `allegro-config --libs` -lm libnet/lib/libnet.a -lpthread
EXE = netacka

all: $(EXE) $(SERVER)

$(EXE): $(OBJECTS_EXE) libnet/lib/libnet.a
	$(CC) $(OBJECTS_EXE) -o $(EXE) $(LIB)

clean:
	rm *.o
	rm libnet/lib/*.a

netacka.o: netacka.h ui.h bots.inc

ui.io: ui.h

net.o: netacka.h

$(BOTS): netacka.h

libnet/lib/libnet.a:
	cp libnet/makfiles/linux.mak libnet/port.mak
	$(MAKE) -C libnet lib
