readme.txt               tenth revision       by gfoot      07/06/2001
______________________________________________________________________

                        |o|   _  _ . 
                        |||-.| |/_)|-
                        '''-'' '`- `-  0.10.11
______________________________________________________________________


This is a file you should read in full before doing anything else.



0. Contents
~~~~~~~~~~~
   0 - Contents
   1 - Disclaimer
   2 - Distribution conditions
   3 - Directory structure
   4 - Installation
   5 - Uninstallation
   6 - Usage instructions
   7 - Contacting the authors



1. Disclaimer
~~~~~~~~~~~~~
   This library is provided free of charge without warranty.  The
   authors hereby disclaim all liability for any damage use or
   abuse of this library may cause, directly or indirectly, to
   anything at all. You use this at your own risk.



2. Distribution conditions
~~~~~~~~~~~~~~~~~~~~~~~~~~
   Libnet is giftware.  You may use, modify, redistribute, and
   generally hack it about in any way you like.  You don't have
   to send the authors anything in return, but of course you're
   welcome to do so.  This could be a complimentary copy of a 
   game, an addition or improvement to Libnet, a bug report, 
   some money (this is particularly encouraged if you use Libnet 
   in a commercial product), or just an email saying "Hi!".  If 
   you redistribute parts of Libnet, or make a game using it, it 
   would be nice if you mentioned Libnet in the credits, but if 
   you just want to pinch a few drivers or techniques then 
   that's fine too.

   However, please do not distribute modified copies under the
   name `Libnet' without stating clearly that they have been
   modified.  Otherwise everybody will just get confused.



3. Directory structure
~~~~~~~~~~~~~~~~~~~~~~
   When you unzipped this you should have told your unzip
   program to recreate the directory structure (e.g. by passing
   the `-d' switch to PKUNZIP; WinZip and InfoZIP do it anyway,
   by default).  If not, you'll have to delete it and start 
   again.

   The Unix version is internally identical to the DOS/Windows 
   version, but comes in a .tar.gz file instead.  Uncompress 
   this in the usual way for your platform, probably either:

       gunzip -cd libnet.tar.gz | tar -xf -

   or, if you have GNU tar:

       tar -xzf libnet.tar.gz

   Having unzipped the library properly, you should have a
   directory called `libnet' with the following subdirectories:

   	docs        -   documentation
   	include     -   holds the libnet.h header file
   	lib         -   library code
   	examples    -   example programs
   	tests       -   miscellaneous test programs
   	batfiles    -   batch files (may be used in building)
   	makfiles    -   makefile stubs (may be used in building)

   The `lib' directory should contain the following 
   subdirectories:
   
   	core        -   core library code
   	drivers     -   driver code
   	include     -   internal header files
   
   The `examples' directory will contain one subdirectory for
   each example program.
   
   The `docs' directory will contain documentation in various
   formats.  You can download an extension archive containing 
   the source to this documentation from the Libnet web page,
   and if you like you can browse the HTML documentation online
   there too.



4. Installation
~~~~~~~~~~~~~~~
   This is a source distribution; before you can use Libnet you
   must build it.  There are two main ways to do this, either of 
   which may not apply on your platform.

   4.1 Building using GNU Make
   """""""""""""""""""""""""""
   This is the only option for Unix and DJGPP, and also works for
   RSXNTDJ and Mingw32.

   First you must configure the build process by creating a file 
   `port.mak' describing your environment.
   
   If you're using DJGPP, RSXNTDJ, Linux, FreeBSD, or Mingw32, 
   just copy the corresponding .mak file from the `makfiles' 
   directory to `port.mak' in the main directory.  You can then 
   edit this if you want to configure it a bit more.

   For Mingw32, you need fileutils, and must set the MINGDIR 
   environment variable as appropriate for your system before 
   running `make install', e.g.

       set MINGDIR=c:/mingw32

   In general on Unix try using either the Linux or FreeBSD 
   versions as a base.  You will need GNU Make, whatever you use, 
   unless you also edit all of the makefiles.  I tested this on
   Digital Unix 4.0 running on a DEC Alpha, and no changes were
   needed to the Linux configuration -- but this machine had GNU
   tools installed.

   To build Libnet, simply run `make' from its base directory 
   (the one with `port.mak' in it).  This will compile everything, 
   producing `libnet.a' in the `lib' directory, various test 
   programs in the `tests' directory and possibly some example 
   programs, in the `examples' heirarchy, depending upon what 
   environment you're working in.

   After building, you can remove intermediate files by
   typing `make clean' from that same directory.

   If something goes badly wrong and you need to rebuild the
   library from scratch, run `make rebuild'.
   
   If you switch to another environment, or just need to build 
   Libnet for more than one environment, run `make cleaner',
   then replace `port.mak', and then rebuild as usual.
   
   4.2 Building using batch files
   """"""""""""""""""""""""""""""
   For MSVC and other (DOSy) platforms that don't have GNU Make,
   you can build Libnet using batch files.

   Zeroth, if you are using MSVC, run `vcvars32.bat', which 
   should have come with the compiler.  Apparently this is 
   necessary to be able to use it from the command line.

   First copy one of the batch files out of `batfiles' into the 
   top directory -- use `msvcmake.bat' for MSVC, `mingmake.bat' 
   for Mingw32, or `rsxmake.bat' for RSXNTDJ.  You might like to 
   rename the batch file to `make.bat' when you copy it.

   Next, run the batch file.  It will build all of Libnet, by
   default.  Alternatively, you can specify `lib', `tests' or 
   `examples' to build only that part of the library.  Note that
   no dependency checking is performed, so make sure you have the 
   library built before building any of the programs.

   4.3 Installing Libnet
   """""""""""""""""""""
   Libnet can install itself on some platforms.  What this means 
   depends upon your environment; for djgpp, this means copying 
   the library file to your djgpp `lib' directory and the header 
   file to your djgpp `include' directory.  For RSXNTDJ the 
   destinations are slightly different -- it's currently set up 
   for my system, but you'll probably want to modify them yourself 
   in `rsxntdj.mak' though.  For Linux, Unix and similar systems, 
   the default destinations (specified in `linux.mak') are 
   `/usr/local/lib' for the library and `/usr/local/include' for 
   the header file.  You probably won't be able to install them 
   here unless you're root.
   
   To perform the installation, type `make install'.



5. Uninstallation
~~~~~~~~~~~~~~~~~
   Uninstallation is possible on platforms where Libnet can 
   install itself.  To completely uninstall the library, run 
   `make veryclean'.  This will remove all object code and 
   executables, and it will delete the library libnet.a and 
   header file libnet.h from wherever you installed them (if 
   you actually did install them).  Again, you probably need 
   root priviledges to remove the installed files on a Unix 
   system.



6. Usage instructions
~~~~~~~~~~~~~~~~~~~~~
   To use the library in your programs, you must:

   - #include <libnet.h> in your source files that need it.  You
     don't need to do this in files that don't call Libnet
     routines and don't use Libnet structures.

   - Link the library (`libnet.a' or `libnet.lib') into your 
     executables.  You can either pass it to the linker like a 
     normal object file, or just put `-lnet' at the end of the 
     linker command line (the one that generates the executable 
     file) if you installed it properly and are using GCC.

   See the docs directory for details of what the functions do,
   or the example programs for further guidance.



7. Contacting the authors
~~~~~~~~~~~~~~~~~~~~~~~~~
   It's best to write to the netgame mailing list, so that more 
   users and developers will read your messages:
   
      netgame@canvaslink.com
   
   If you're not a member of the list, make this clear in your 
   message, otherwise you might not see the replies.  For 
   subscription instructions see the main documentation.

   For queries about specific drivers, please see the
   documentation for the driver in question.



