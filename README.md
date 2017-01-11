# Netacka

Netacka is a multiplayer game similar to
[Zatacka](https://en.wikipedia.org/wiki/Achtung,_die_Kurve!).

For more information, go to https://pwmarcz.pl/netacka.

I wrote this in high school. Expect some horrible code.

## Building

Netacka depends on:

- [Allegro](http://liballeg.org/) 4 (under Ubuntu, `liballegro4-dev`)
- [libnet](http://libnet.sourceforge.net/)

You might have some trouble with `libnet`, it's currently unmaintained and you
will have to install it from source. I really should port this to something
else.

The provided Makefile should run under Linux.
