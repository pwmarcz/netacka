rdmchat demo readme.txt  first revision       by gfoot        01/04/99
______________________________________________________________________

                            |o|   _  _ . 
                            |||-.| |/_)|-
                            '''-'' '`- `-

    libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
______________________________________________________________________


This file documents the RDM client-server chat program example.



0. Contents
~~~~~~~~~~~
   0 - Contents
   1 - In brief
   2 - More information
   3 - Platforms and portability



1. In brief
~~~~~~~~~~~
   The `rdmchat' example program consists of two executables -- the 
   client and the server.

   To start the server, simply run the `server' program.  It will 
   list the available drivers; select one by number and the server 
   will start.

   Clients can connect using the `client' program; it, too, asks 
   the user to choose a driver, and it asks for the server's 
   address and the user's nickname.  After this it tries to 
   communicate with the server; if this succeeds the client is 
   connected.  If it's not getting anywhere, pressing a key will 
   abort.

   Connected clients get an IRC-like display, showing other 
   clients joining and leaving and what each client says (prefixed 
   by the speaker's nickname).  Clients speak by typing; when 
   they hit Enter the text is sent to the server, which relays 
   it on to all the clients.

   If the line begins with a `/', the server interprets it as a 
   command.  The only command supported at present is "/quit", 
   which asks the server to disconnect the client.

   All messages are echoed to the server's console.  Pressing a 
   key on the console causes the server to quit.



2. More information
~~~~~~~~~~~~~~~~~~~
   (not written yet)


3. Platforms and portability
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   I'll just briefly mention which platforms this supports.  It was
   originally written in DOS.  Libnet's API changed slightly since
   that release, and I started developing in Linux instead of DOS.
   In doing this, I have now wrapped the DOS conio functions I used 
   with ncurses equivalents.  The Linux version works fine.  The DOS
   version *should* still work but I haven't tested it.  The program 
   is disabled in other configurations, but ought to work on any Unix
   with ncurses (or curses I think -- not sure).  It won't work with
   RSXNTDJ or MSVC until someone who knows about Windows writes a 
   front end.

   Lastly, some testing suggestions, based around the Internet drivers.  
   If your OS can multitask to some degree, you should be able to test
   this locally by running a server as one task, and clients as other 
   tasks.  Tell the clients to connect to 127.0.0.1.  This worked for
   me in Windows once upon a time, and works better in Linux.  In
   Windows, make sure the "Always suspend" box in your DOS prompt's 
   properties is clear, otherwise the server will be suspended when 
   it's not in the foreground.

   This local testing is particularly useful if you want to 
   test things without having immediate access to the Internet 
   -- for instance, for half the year I pay by the minute to use 
   the telephone line -- not exactly great conditions in which 
   to write network applications!  By using 127.0.0.1 (the loopback 
   IP address) I can test things to some extent.  Of course nothing 
   beats a LAN.


