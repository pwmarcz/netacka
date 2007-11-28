
.PHONY = all clean run

BOTS = bot.o genialny_bot0.o genialny_bot.o
OBJECTS_EXE = netacka.o net.o $(BOTS)

CC = gcc
CFLAGS = -s -O2 -Wall
#CFLAGS = -g3

# change these for other systems
LIB = -lalleg -lm -lnet -lwsock32
EXE = netacka.exe

all: $(EXE)

$(EXE): $(OBJECTS_EXE)
	$(CC) -s $(OBJECTS_EXE) -o $(EXE) $(LIB) -mwindows

clean:
	del *.o

# for SciTE; I set "make run" for the run command
run: $(EXE)
	$(EXE)
# change to ./$(EXE) if needed

netacka.o: net.h bots.inc 

net.o: net.h 

$(BOTS): net.h
