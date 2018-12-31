@echo off
gcc -DTARGET_MINGW32 -O2 -g -I../include -Iinclude -c %1.c -o %1.o

