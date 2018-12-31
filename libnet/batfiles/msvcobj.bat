@echo off
cl /DTARGET_MSVC /MD /c /O2 /nologo /I../include /Iinclude %1.c /Fo%1.obj

