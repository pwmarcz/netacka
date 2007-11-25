/* BOTS.INC */

#include "net.h"

// bots by gawron - bot.c
void *start_bot (int m);
int check_bot (BITMAP *arena, int m,void *data);
void close_bot (void *data);

// bots by derezin - genialny_bot{0}.c
void *start_bot_d0 (int m);
int check_bot_d0 (BITMAP *arena,int m,void *data);
void close_bot_d0 (void *data);

void *start_bot_d (int m);
int check_bot_d (BITMAP *arena,int m,void *data);
void close_bot_d (void *data);

const struct bot {
   char *name;
   char *descr;
   void *((*start)()); // void start ();
   int (*check)(BITMAP*,int,void*); // int check (BITMAP *arena,int m);
   void (*close)(void*); // void close ();
} bots[] = {
   { "sigma","by jamiwron",
      &start_bot,&check_bot,&close_bot } ,
   { "pi","by derezin",
      &start_bot_d0,&check_bot_d0,&close_bot_d0 } /*,
   { "genialny dwa","by derezin",
      &start_bot_d,&check_bot_d,&close_bot_d }*/
};

const int N_BOTS=sizeof(bots)/sizeof(struct bot);
