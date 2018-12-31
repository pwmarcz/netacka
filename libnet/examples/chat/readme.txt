chat demo readme.txt     first revision       by gfoot        17/01/99
______________________________________________________________________

                            |o|   _  _ . 
                            |||-.| |/_)|-
                            '''-'' '`- `-

    libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
______________________________________________________________________


This file documents the client-server chat program example.



0. Contents
~~~~~~~~~~~
   0 - Contents
   1 - In brief
   2 - More information
   3 - Platforms and portability



1. In brief
~~~~~~~~~~~
   The `chat' example program consists of two executables -- the 
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
   The server works by first opening an input channel -- that is,
   a channel with a default local binding.  You do this by passing 
   the empty string to the `net_openchannel' function.  Since the
   binding is known (by Libnet internally, not by you or the user),
   it's easy for the client to connect to -- it too knows what 
   binding would have been used.  Unless the user overrides the 
   binding (by being really specific when asked for the address to
   connect to), Libnet will assume the default.  In Internet terms,
   the default binding is a specific port (taken from the config 
   file, or the default that's compiled into the library).  If the 
   user of the client specifies a port along with the IP, that will
   be used; otherwise the default port will be used.

   Next the server loops constantly, checking for things that need
   doing.  One thing it needs to check is whether there are any
   messages on the input channel.  When a connection attempt is 
   received from a client, the server opens a fresh channel for 
   general communication with that client.  It could use the input 
   channel for all this but it would get tedious to change the 
   target all the time.  The new channel is used to send a message 
   to the return address of the connection request -- i.e. the 
   channel on the client that sent the request.  The server 
   dispatches a message to all clients to announce the newcomer's 
   presence, and all goes on as normal.  

   The other thing it does in its loop, is poll the list of connected 
   clients; each has a node in a linked list, where the clients' 
   nicknames and channels are stored, for instance.  If something is 
   said, it processes the message and, unless it's a command, sends 
   it on to all the clients.

   Those in the know will see that this is similar to the way IRC 
   operates; the situation is more complicated there since there 
   are many servers networked together, but the basic principles 
   are the same.



3. Platforms and portability
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   I'll just briefly mention which platforms this supports.  It was
   originally written in DOS.  Libnet's API changed slightly since
   that release, and I started developing in Linux instead of DOS.
   In doing this, I have now wrapped the DOS conio functions I used 
   with ncurses equivalents.  The Linux version works fine.  The DOS
   version *should* still work but I haven't tested it.  The program 
   is disabled in other configurations, but ought to work on any Unix
   with ncurses (or curses I think -- not sure).  I don't know whether
   or not it will work with RSXNTDJ or MSVC, sorry.

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


