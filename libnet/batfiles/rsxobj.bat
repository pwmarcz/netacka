@echo off
gcc -DTARGET_RSXNTDJ -O2 -g -I../include -Iinclude -c %1.c -o %1.o

