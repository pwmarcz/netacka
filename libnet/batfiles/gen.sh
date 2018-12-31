#!/bin/sh

# This script generates the generic building batch files for Windows systems

echo "@echo off" > makelib.bat
echo "cd ..\\lib" >> makelib.bat
( cd ../lib
	for f in */*.c; do
		echo "call ..\\batfiles\\obj.bat `echo $f | sed -e 's/\\.c//' -e 's/\\//\\\\/g'`"
	done
) >> makelib.bat
echo "call ..\\batfiles\\lib.bat libnet core\\* drivers\\*" >> makelib.bat
echo "cd ..\\batfiles" >> makelib.bat

echo "@echo off" > maketest.bat
echo "cd ..\\tests" >> maketest.bat
( cd ../tests
	for f in *.c; do
		echo "call ..\\batfiles\\exe.bat `echo $f | sed -e 's/\\.c//'`"
	done
) >> maketest.bat
echo "cd ..\\batfiles" >> maketest.bat

echo "@echo off" > makeex.bat
echo "echo Sorry, the example programs have not been ported to Windows yet." >> makeex.bat

echo "@echo off" > makeall.bat
echo "call makelib.bat" >> makeall.bat
echo "call maketest.bat" >> makeall.bat
echo "call makeex.bat" >> makeall.bat

