@echo off
cl /DTARGET_MSVC /MD /O2 /nologo /I../include %1.c ../lib/libnet.lib wsock32.lib

