
.PHONY = all clean run

BOTS = bot.o genialny_bot0.o genialny_bot.o
OBJECTS_EXE = netacka.o net.o $(BOTS)

CC = gcc
CFLAGS = -s -O2 -Wall -fgnu89-inline
#CFLAGS = -g3 -fgnu89-inline

# change these for other systems
LIB = `allegro-config --libs` -lm -lnet -lpthread
EXE = netacka

all: $(EXE) $(SERVER)

$(EXE): $(OBJECTS_EXE)
	$(CC) $(OBJECTS_EXE) -o $(EXE) $(LIB)

clean:
	rm *.o

# for SciTE; I set "make run" for the run command
run: $(EXE)
	./$(EXE)

netacka.o: netacka.h bots.inc

net.o: netacka.h

$(BOTS): netacka.h
