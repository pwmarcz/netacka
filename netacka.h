#ifndef _NETACKA_H
#define _NETACKA_H

#include <stdlib.h>
#include <libnet.h>
#include <allegro5/allegro.h>

#define VER_MAJOR 4
#define VER_MINOR 0
#define VER_STRING "4.0"

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

#define gNORMAL    0
#define gONEFINGER 1
#define gTRON      2


#define CLIENT_PLAYERS 6

#define MAX_CLIENTS 23

#define MAX_PLAYERS MAX_CLIENTS


#define cBLACK 0
#define cDARKGRAY 1
#define cVLIGHTGRAY 2
#define cLIGHTGRAY 3
#define cRED 4
#define cGRAY 7
#define cBLUE 9
#define cGREEN 10
#define cCYAN 11
#define cORANGE 12
#define cPINK 13
#define cYELLOW 14
#define cWHITE 15
#define cWHITE_WALL 16


extern int game_mode, torus;
extern int screen_w,screen_h;

extern struct player {
   char name[11];
   int x,y,old_x,old_y;
   int hole,old_hole,to_change;
   int a,last_da,da,da_change_time;
   int playing,alive;
   int client,client_num;
   int score;
} players[];

int _test(ALLEGRO_BITMAP *arena,int old_x,int old_y,int x,int y,
                 int hole,int old_hole);
void _update(int x,int y,int a,int *x1,int *y1);
void _update_tron(int x,int y,int a,int *x1,int *y1);
void _put(ALLEGRO_BITMAP *arena,int x,int y,ALLEGRO_COLOR c);
static inline void _update_angle(int *a,int da)
{
   *a=(*a+256+4*da)%256;
}

extern int net_driver;
int start_net();
void send_byte(NET_CHANNEL *chan,unsigned char a);
void rect(ALLEGRO_BITMAP *bitmap, int x1, int y1, int x2, int y2, ALLEGRO_COLOR c);
void rectfill(ALLEGRO_BITMAP *bitmap, int x1, int y1, int x2, int y2, ALLEGRO_COLOR c);
int test_pixel(ALLEGRO_BITMAP *bitmap, int x, int y);

#endif
