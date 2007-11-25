#include <stdio.h>
#include <stdlib.h>
#include <allegro.h>
#include <stdio.h>
#include <math.h>

#include "net.h"

#define SEARCHING_DISTANCE 60

const char *BOT_NAME = "gawron";

static int longest_path (BITMAP *arena, int x, int y, int a, int da)
{
   int d=0,l=SEARCHING_DISTANCE;
   int x1,y1;
   if(game_mode==gTRON) a+=da;
   while(d++,l--)
   {
    if(game_mode!=gTRON) 
    {
      _update_angle(&a,da);
      _update(x,y,a,&x1,&y1);
    }
    else
      _update_tron(x,y,a,&x1,&y1);
    if (_test (arena,x,y,x1,y1,0,0)) break;
    x=x1; y=y1;
   }
   return d;
}

/* Kod pozostawiony ku przestrodze - jako przyklad POTWORNOSCI.
   Tak wlasnie nie nalezy pisac w C. */
/*int t_test(BITMAP *arena, int x, int y, int a)
{
    if (torus)
    {
      switch (a)
      {
        case 0:return (getpixel(arena, (x+screen_w-112)%(screen_w-111)+1,  (y+screen_h-1)%(screen_h-2)+1) ||
                      getpixel(arena,  (x+screen_w-111)%(screen_w-111)+1,  (y+screen_h-1)%(screen_h-2)+1) ||
                      getpixel(arena,  (x+screen_w-112)%(screen_w-111)+1,  (y+screen_h+1)%(screen_h-2)+1) ||
                      getpixel(arena,  (x+screen_w-111)%(screen_w-111)+1,  (y+screen_h+1)%(screen_h-2)+1) );
        case 1:return (getpixel(arena, (x+screen_w-110)%(screen_w-111)+1,  (y+screen_h-3)%(screen_h-2)+1) ||
                      getpixel(arena,  (x+screen_w-110)%(screen_w-111)+1,  (y+screen_h-2)%(screen_h-2)+1) ||
                      getpixel(arena,  (x+screen_w-108)%(screen_w-111)+1,  (y+screen_h-3)%(screen_h-2)+1) ||
                      getpixel(arena,  (x+screen_w-108)%(screen_w-111)+1,  (y+screen_h-2)%(screen_h-2)+1) );
        case 2:return (getpixel(arena, (x+screen_w-112)%(screen_w-111)+1,  (y+screen_h-4)%(screen_h-2)+1) ||
                      getpixel(arena,  (x+screen_w-111)%(screen_w-111)+1,  (y+screen_h-4)%(screen_h-2)+1) ||
                      getpixel(arena,  (x+screen_w-112)%(screen_w-111)+1,  (y+screen_h-6)%(screen_h-2)+1) ||
                      getpixel(arena,  (x+screen_w-111)%(screen_w-111)+1,  (y+screen_h-6)%(screen_h-2)+1) );
        default:return(getpixel(arena, (x+screen_w-113)%(screen_w-111)+1,  (y+screen_h-3)%(screen_h-2)+1) ||
                      getpixel(arena,  (x+screen_w-113)%(screen_w-111)+1,  (y+screen_h-2)%(screen_h-2)+1) ||
                      getpixel(arena,  (x+screen_w-115)%(screen_w-111)+1,  (y+screen_h-3)%(screen_h-2)+1) ||
                      getpixel(arena,  (x+screen_w-115)%(screen_w-111)+1,  (y+screen_h-2)%(screen_h-2)+1) );
      }
    }
    switch (a)
    {
        case 0:return (getpixel(arena,x,y+2)||getpixel(arena,x+1,y+2)||
            getpixel(arena,x,y+4)||getpixel(arena,x+1,y+4));
        case 1:return (getpixel(arena,x+2,y)||getpixel(arena,x+2,y+1)||
            getpixel(arena,x+4,y)||getpixel(arena,x+4,y+1));
        case 2:return (getpixel(arena,x,y-1)||getpixel(arena,x+1,y-1)||
            getpixel(arena,x,y-3)||getpixel(arena,x+1,y-3));
        default:return(getpixel(arena,x-1,y)||getpixel(arena,x-1,y+1)||
            getpixel(arena,x-3,y)||getpixel(arena,x-3,y+1));
    }
}

int longest_path_t (BITMAP *arena, int x, int y, int a, int l)
{
    if ( t_test (arena,x,y,a)) return 0;
    int x1=x,y1=y;
    switch (a)
    {
        case 0:y1+=3;break;
        case 1:x1+=3;break;
        case 2:y1-=3;break;
        case 3:x1-=3;break;
    }
    
    if (l>=6) return 1;
    return (longest_path_t(arena,x1,y1,a,l+1)+1);
}*/


static inline int find_the_way_n (BITMAP *arena, int x, int y, int a,int spaczenie)
{
    int l0,l1,l2;
    l0=longest_path(arena,x,y,a,-1);
    l1=longest_path(arena,x,y,a,0);
    l2=longest_path(arena,x,y,a,1);
    if (l0==l2 && l0>l1) return spaczenie;
    if (l0>l2 && l0>l1) return -1;
    if (l2>l0 && l2>l1) return 1;
    return 0;
}

static inline int find_the_way_o (BITMAP *arena, int x, int y, int a,int spaczenie)
{
    int l0=longest_path(arena,x,y,a,-1),
        l2=longest_path(arena,x,y,a,1);
    if(l0==l2) return spaczenie;
    if(l0>l2) return -1;
    return 1;
}

static inline int find_the_way_t(BITMAP *arena, int x, int y, int a, int last_da,int spaczenie)
{
    int l0,l1,l2;
    l1=longest_path(arena,x,y,a,0);
    if (last_da!=-1) 
       l0=longest_path(arena,x,y,(a+3)%4,0);
    else l0=0;
    if (last_da!=1) 
       l2=longest_path(arena,x,y,(a+1)%4,0);
    else l2=0;
    if (l0==l2 && l0>l1) return spaczenie;
    if (l0>l2 && l0>l1) return -1;
    if (l2>l0 && l2>l1) return 1;
    return 0;
}

int check_bot (BITMAP *arena, int m,void *data)
{
   int spaczenie=(m%2)?-1:1;
    switch(game_mode)
   {
      case gONEFINGER:
       return find_the_way_o (arena,players[m].x,players[m].y,players[m].a,spaczenie)==1?1:0;
      case gTRON:
       return find_the_way_t (arena,players[m].x,players[m].y,players[m].a,
          players[m].last_da,spaczenie);
      default:
       return find_the_way_n (arena,players[m].x,players[m].y,players[m].a,spaczenie);
   }
}

static int dummy[]={0};

void *start_bot (int m) 
{  
  //fprintf(stderr,"start %d\n",m);
  return &dummy;
}
   
void close_bot (void *data) 
{
  //fprintf(stderr,"close\n");
}
