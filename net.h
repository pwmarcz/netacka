/* NET.H */

#ifndef _NET_H
#define _NET_H

#include <stdlib.h>
#include <libnet.h>

#define VER_MAJOR 2
#define VER_MINOR 1
#define VER_STRING "2.1"

#define clIWANNAPLAY      1
#define seHEREARESETTNGS  2
#define seHEREAREPLAYERS  3
#define clMYINPUT         4
#define clGOODBYE         5
#define seNEWROUND        6
#define clGOTIT           7
#define clREADY           8
#define seIKICKYOU        9
#define seIMFULL         14
#define seKONEC_HRY      18
#define seBADPASSWD      20
#define csANAME          21

#define PING             10
#define PONG             11

#define clKNOCKKNOCK     12
#define seMYINFO         19

#define seREGISTERME     13
#define seIMOFF          16
#define clGIMMESERVERS   15
#define loASERVER        17
#define seLOGTHIS        22

extern int net_driver;

int start_net();

void send_byte(NET_CHANNEL *chan,unsigned char a);

#endif
