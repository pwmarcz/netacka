#include <stdlib.h>
#include <allegro.h>
#include <stdio.h>
#include <math.h>
#include "netacka.h"

#define JAK_DALEKO 100
#define USREDNIAC 3
#define ELO 10

static int tab1[2*64+1];
static int tab2[2*64+1];
static int indeks, max, przeszlismy, gdzie_max;
static int i,alfa;

static BITMAP *arena2=NULL;


static inline int na_wprost(BITMAP *arena,BITMAP *arena2, int x, int y, int a){
  int d=0,l=JAK_DALEKO,xa,ya;
  while(d++,l--)
     {
     _update(x,y,a,&xa,&ya);
     if(_test(arena,x,y,xa,ya,0,0) || _test(arena2,x,y,xa,ya,0,0)) break;
     x=xa; y=ya;
   }
   return d;
}

static inline int find_the_way_n (BITMAP *arena,BITMAP *arena2,int m, int x, int y, int a)
{


    clear_bitmap(arena2);



    for (i=0; i<MAX_CLIENTS; i++)
      {
	if (players[i].alive && players[i].playing && i!=m)
	  {
	    int xi,yi,ai,xip,yip;
	    xi=players[i].x;
	    yi=players[i].y;
	    ai=players[i].a;
	    for(alfa=1; alfa<ELO; alfa++){
	      ai-=4;
	      _update(xi,yi,ai,&xip,&yip);
	      if(_test(arena,xi,yi,xip,yip,0,0) ) break;
	      _put(arena2,xip>>8,yip>>8,alfa);
	      xi=xip; yi=yip;

	    }
	    xi=players[i].x;
	    yi=players[i].y;
	    ai=players[i].a;
	    for(alfa=1; alfa<ELO; alfa++){
	      ai+=4;
	      _update(xi,yi,ai,&xip,&yip);
	      if(_test(arena,xi,yi,xip,yip,0,0) ) break;
	      _put(arena2,xip>>8,yip>>8,alfa);
	       xi=xip; yi=yip;
	    }
	    xi=players[i].x;
	    yi=players[i].y;
	    ai=players[i].a;
	    for(alfa=1; alfa<ELO; alfa++){
	      _update(xi,yi,ai,&xip,&yip);
	      if(_test(arena,xi,yi,xip,yip,0,0) ) break;
	      _put(arena2,xip>>8,yip>>8,alfa);
	       xi=xip; yi=yip;
	    }
	  }
      }
    przeszlismy=0;
    tab1[0]=na_wprost(arena,arena2,x,y,a);
    indeks=1;
    int x1,y1,a1,xa,ya;
    x1=x;y1=y;a1=a;
    while(indeks<=64)
    {
	_update_angle(&a1,-1);
	_update(x1,y1,a1,&xa,&ya);

	if(_test(arena,x1,y1,xa,ya,0,0) || _test(arena2,x1,y1,xa,ya,0,0) ) break;
	przeszlismy++;
	x1=xa; y1=ya;
	tab1[indeks]=na_wprost(arena,arena2,x1,y1,a1)+przeszlismy;
	indeks++;
    }
    int srodek=indeks-1;
    for(i=0; i<indeks; i++) tab2[i]=tab1[indeks-i-1];

    x1=x; y1=y; a1=a;
    przeszlismy=0;
    while(indeks<=128)
    {
      	_update_angle(&a1,1);

	_update(x1,y1,a1,&xa,&ya);
	if(_test(arena,x1,y1,xa,ya,0,0) || _test(arena2,x1,y1,xa,ya,0,0)) break;
	przeszlismy++;
	x1=xa; y1=ya;
	tab2[indeks]=na_wprost(arena,arena2,x1,y1,a1)+przeszlismy;
	indeks++;
    }
    //jak odkomentujesz ponizszy komentarz to uzyskasz zupelnie innego bota
    /*int suma1=0;
    int suma2=0;
    for (i=0; i<srodek; i++) suma1+=tab2[i];
    for (i=srodek+1; i<indeks; i++) suma2+=tab2[i];

    if (suma2>suma1) return 1;
    if (suma1>suma2) return -1;
    return 0;*/
    if(indeks>=USREDNIAC*2-1){
      int suma=0; for (i=0; i<USREDNIAC*2-1; i++) suma+=tab2[i];
      max=suma;
      gdzie_max=USREDNIAC-1;
      //max+=2*tab2[gdzie_max];
      for(i=USREDNIAC; i<indeks-USREDNIAC+1; i++){
	  suma+=tab2[i+USREDNIAC-1]/*+2*tab2[i]-2*tab2[i-1]*/-tab2[i-USREDNIAC];
	if(suma>=max){max=suma; gdzie_max=i;}
      }
      /*max=-1; gdzie_max=0;
	while(indeks--)
	if(tab2[indeks]>max) { max=tab2[indeks]; gdzie_max=indeks;}*/
      //fprintf(stderr,"\n");
      /*int fiks=gdzie_max;
      max=tab2[gdzie_max];
      for (i=fiks-USREDNIAC+1; i<fiks+USREDNIAC; i++)
      {
	  if (tab2[i]>max)
	  {
	      gdzie_max=i;
	      max=tab2[i];
	  }
	  }*/
      /*printf("srodek=%d indeks=%d gdzie_max=%d droga= %d \n",srodek,indeks,gdzie_max,tab2[gdzie_max]);*/
      if(gdzie_max>srodek) return 1;
      if(gdzie_max<srodek) return -1;
      return 0;
    }
    else return 0;
}

static int dummy[]={0};

int check_bot_d0 (BITMAP *arena, int m, void *data)
{
  return find_the_way_n (arena,arena2,m,players[m].x,players[m].y,players[m].a);
}


void *start_bot_d0 (int m)
{
  if(!arena2)
    arena2=create_bitmap(screen_w-110,screen_h);
  return &dummy;
}

void close_bot_d0 (void *data)
{
  if(arena2)
    {
      destroy_bitmap(arena2);
      arena2=NULL;
    }
}
