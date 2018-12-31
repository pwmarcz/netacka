@echo off
gcc -DTARGET_RSXNTDJ -O2 -g -I../include %1.c ../lib/libnet.a -lwsock32 -o %1.exe

