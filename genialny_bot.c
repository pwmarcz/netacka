#include <stdlib.h>
#include <allegro.h>
#include <stdio.h>
#include <math.h>
#include "net.h"

#define JAK_DALEKO 100
#define USREDNIAC 3
#define WROG 16
#define STALA 3
#define KASOWANIE MAX_PLAYERS * WROG * STALA
#define PROSTO 0
#define LEWO -1
#define PRAWO 1
#define BEZWLADNOSC 0
#define SKOK 1
typedef struct punkt {
    int x;
    int y;
    int a;
} punkt;
static punkt gdzie_wrog[MAX_PLAYERS][3][WROG];
static int zyje_wrog[MAX_PLAYERS][3][WROG];
static punkt doskasowania[KASOWANIE]; 
static int chwile[WROG];
static int wiazkapom[70];
static int wiazka[130];
static int przeszlismy,ile_kasuj,indeks,max,gdzie_max;
static int killed,kiedy_killed,morduj;
static punkt pkt;
static punkt bot;
static punkt gdzie_bot[JAK_DALEKO];
static int nr;
static BITMAP *arena2=NULL;
static int wynik;
typedef struct moje_dane {
    int kier;
} moje_dane;
static inline void rysuj (int x,int y,int c)
{
    if (ile_kasuj>=KASOWANIE-1) return;
    _put(arena2,x>>8,y>>8,c);
    pkt.x=x; pkt.y=y;
    doskasowania[ile_kasuj]=pkt;
    ile_kasuj++;
}
static inline void inicjuj ()
{
    int dzeta,delta,teta;
    killed=0;
    kiedy_killed=-1;
    ile_kasuj=0;
    wynik=-2;
    bot.x=players[nr].x;
    bot.y=players[nr].y;
    bot.a=players[nr].a;
    gdzie_bot[0]=bot;
    chwile[0]=0;
    przeszlismy=0;
    for (dzeta=0; dzeta<MAX_PLAYERS; dzeta++)
    {
	for (delta=0; delta<3; delta++)
	{
	    gdzie_wrog[dzeta][delta][0].x=players[dzeta].x;
	    gdzie_wrog[dzeta][delta][0].y=players[dzeta].y;
	    gdzie_wrog[dzeta][delta][0].a=players[dzeta].a;
	    zyje_wrog[dzeta][delta][0]=players[dzeta].alive;
	    for (teta=1; teta<WROG; teta++)
		zyje_wrog[dzeta][delta][teta]=0;
	}
    }
}
static inline void czysta_arena ()
{
    int dzeta,delta;
    //masked_blit(arena2,screen,0,0,0,0,screen_w,screen_h);
    for (dzeta=0; dzeta < ile_kasuj; dzeta++) 
	_put(arena2, (doskasowania[dzeta].x)>>8, (doskasowania[dzeta].y)>>8, 0);
    ile_kasuj=0;
    for (dzeta=0; dzeta<MAX_PLAYERS; dzeta++)
    {
	for (delta=0; delta<3; delta++)
	{
	gdzie_wrog[dzeta][delta][0].x=players[dzeta].x;
	gdzie_wrog[dzeta][delta][0].y=players[dzeta].y;
	gdzie_wrog[dzeta][delta][0].a=players[dzeta].a;
	zyje_wrog[dzeta][delta][0]=players[dzeta].alive;
	}
    }
    bot.x=players[nr].x;
    bot.y=players[nr].y;
    bot.a=players[nr].a;
    gdzie_bot[0]=bot;
    chwile[0]=0;
    przeszlismy=0;
    killed=0;
    kiedy_killed=-1;
} 
static inline void rusz_wrogow (BITMAP *arena)
{
    if (przeszlismy>=WROG-1) return;
    int dzeta,delta,xif,yif;
    int pu[3];
    for (dzeta=0; dzeta < MAX_PLAYERS; dzeta++)
    {
	pu[0]=0; pu[1]=0; pu[2]=0;
	for (delta=0; delta<3; delta++)
	{
	    if ((dzeta != nr) && zyje_wrog[dzeta][delta][przeszlismy])
	    {
		gdzie_wrog[dzeta][delta][przeszlismy+1].a=gdzie_wrog[dzeta][delta][przeszlismy].a;
		_update_angle(&gdzie_wrog[dzeta][delta][przeszlismy+1].a,delta-1);
		_update(gdzie_wrog[dzeta][delta][przeszlismy].x, 
			gdzie_wrog[dzeta][delta][przeszlismy].y, 
			gdzie_wrog[dzeta][delta][przeszlismy].a, &xif, &yif);
		if (_test(arena2, gdzie_wrog[dzeta][delta][przeszlismy].x, 
			  gdzie_wrog[dzeta][delta][przeszlismy].y, xif, yif, 0, 0)==1 && delta==1)
		{
		    killed=1;
		    if (kiedy_killed==-1) kiedy_killed=przeszlismy;
		    continue;
		}
		if (_test(arena, gdzie_wrog[dzeta][delta][przeszlismy].x, 
			  gdzie_wrog[dzeta][delta][przeszlismy].y, xif, yif, 0, 0)/* ||
		   ( _test(arena2, gdzie_wrog[dzeta][delta][przeszlismy].x, 
			  gdzie_wrog[dzeta][delta][przeszlismy].y, xif, yif, 0, 0) &&
		     _test(arena2, gdzie_wrog[dzeta][delta][przeszlismy].x, 
		     gdzie_wrog[dzeta][delta][przeszlismy].y, xif, yif, 0, 0)!=dzeta*10+delta+2)  */)
		{
		    zyje_wrog[dzeta][delta][przeszlismy+1]=0;
		    zyje_wrog[dzeta][delta][przeszlismy]=0;		    
		}
		else 
		{
		    pu[delta]=1;
		    rysuj(xif,yif,dzeta*10+delta+2);
		    gdzie_wrog[dzeta][delta][przeszlismy+1].x=xif;
		    gdzie_wrog[dzeta][delta][przeszlismy+1].y=yif;
		    zyje_wrog[dzeta][delta][przeszlismy+1]=1;
		}
		
	    }
	}
//	for (delta=0; delta<3; delta++) 
	//  if (pu[delta]) rysuj(xif,yif,2);
    }
}
static inline int idz(BITMAP *arena,int zm_kier,int dyst)
{
    int d=0,len=dyst,xa,ya;
    while(len && przeszlismy<JAK_DALEKO-1 && wynik==-2)
    { 
	_update_angle(&bot.a,zm_kier);
	_update(bot.x,bot.y,bot.a,&xa,&ya);
	//if (_test(arena2,bot.x,bot.y,xa,ya,0,0)==2 && !_test(arena,bot.x,bot.y,xa,ya,0,0)) printf("(%d, %d,kier=%d)",(bot.x-players[m].x)>>8,(bot.y-players[m].y)>>8,zm_kier);
	//else printf("0");
	if(_test(arena,bot.x,bot.y,xa,ya,0,0) || _test(arena2,bot.x,bot.y,xa,ya,0,0)) break;
	bot.x=xa; bot.y=ya;
	if (przeszlismy < WROG-1) 
	{
	    rusz_wrogow(arena);
	    rysuj(bot.x,bot.y,1);
	    chwile[przeszlismy+1]=ile_kasuj;
	}
	gdzie_bot[przeszlismy+1]=bot;
	przeszlismy++;
	len--;
	d++;
    }
    return d;
}
static inline void cofnij_do(int chwila)
{
    int dzeta;
    if ((chwila>=przeszlismy) || (chwila<0)) return;
    przeszlismy=chwila;
    bot=gdzie_bot[chwila];
    if (kiedy_killed>chwila) 
    {
	kiedy_killed=-1;
	killed=0;
    }
    if (chwila<WROG-1)
    {
	for (dzeta=ile_kasuj-1; dzeta >= chwile[chwila]; dzeta--) 
	    _put(arena2, (doskasowania[dzeta].x)>>8, (doskasowania[dzeta].y)>>8, 0);
	ile_kasuj=chwile[chwila];
    }
}
static inline int wybierz(int abc1, int abc2, int abc3)
{
    if ((abc1>=abc2) && (abc1>=abc3)) return LEWO;
    return ((abc2 >= abc3) ? PROSTO : PRAWO);    
}
static inline void zwroc(int w) 
{ 
    if (wynik==-2) wynik=w;
}
static inline int find_the_way_n (BITMAP *arena,moje_dane *dane)
{
    inicjuj();    
    wiazkapom[0]=idz(arena,PROSTO,JAK_DALEKO);
    cofnij_do(0);
    int i;
    indeks=1;
    morduj=0;
    while (indeks<=64)
    {
	if (idz(arena,LEWO,SKOK)==SKOK)
	{
	    int ch=przeszlismy;
	    wiazkapom[indeks]=idz(arena,PROSTO,JAK_DALEKO);
	    if (przeszlismy==JAK_DALEKO) wiazkapom[indeks]*=10;
	    if (killed) morduj=LEWO; 
	    indeks++;
	    cofnij_do(ch);
	} else break;
    }
    cofnij_do(0);
    int srodek=indeks-1;
    //printf("<--------->");
    for (i=0; i<indeks; i++){ wiazka[i]=wiazkapom[indeks-i-1]; /*printf("%d ",wiazka[i]);*/ }
    //if (bot.x==players[m].x) printf("=srodek");
    while (indeks<=128)
    {
	if (idz(arena,PRAWO,SKOK)==SKOK)
	{
	    int ch=przeszlismy;	    
	    wiazka[indeks]=idz(arena,PROSTO,JAK_DALEKO);
	    if (przeszlismy==JAK_DALEKO) wiazka[indeks]*=10;
	    //printf("%d ",wiazka[indeks]);
	    indeks++;
	    if (killed) morduj=PRAWO; 
	    cofnij_do(ch);
	} else break;
    }
    if (indeks-srodek>=32 && srodek>=32 && morduj!=0) { /*printf("kill\n");*/ zwroc(morduj); }
    //printf("\n");
    //if (wiazka[srodek]==JAK_DALEKO) zwroc(PROSTO);
    if(indeks>=USREDNIAC*2-1)
      {
	int suma=0; for (i=0; i<USREDNIAC*2-1; i++) suma+=wiazka[i];
	max=suma;
	gdzie_max=USREDNIAC-1;
	//max+=2*wiazka[gdzie_max];
	for(i=USREDNIAC; i<indeks-USREDNIAC+1; i++)
	  {
	    suma+=wiazka[i+USREDNIAC-1]/*+2*wiazka[i]-2*wiazka[i-1]*/-wiazka[i-USREDNIAC];
	    if(suma>=max)
	      {
		max=suma; 
		gdzie_max=i;
	      }
	  }
	if(gdzie_max>srodek) zwroc(PRAWO);
	if(gdzie_max<=srodek) zwroc(LEWO);
	//zwroc(PROSTO);
      }
    else 
      {
	max=-1;
	gdzie_max=0;
	for (i=0; i<indeks; i++) 
	  if (wiazka[indeks]>max) 
	    {
	      max=wiazka[indeks];
	      gdzie_max=indeks;
	    }
	if(gdzie_max>srodek) zwroc(PRAWO);
	if(gdzie_max<=srodek) zwroc(LEWO);
      }
    czysta_arena();
    //dane->kier=wynik;
    return wynik;
}

int check_bot_d (BITMAP *arena, int m, void *data)
{
    moje_dane *dane=data;
    nr=m;
    /* cos tam mozesz z tym robic, np. przekazac do find_the_way_n... */
    int wynik=find_the_way_n (arena,dane);
    if (game_mode==gONEFINGER) return (wynik>=0)?1:0;
    else return wynik;
}


void *start_bot_d (int m) 
{
   moje_dane *dane=malloc(sizeof(moje_dane));
   if(!dane) return NULL;
      /* tu inicjuj strukture */
   dane->kier=0;
  if(!arena2)
    clear_bitmap(arena2=create_bitmap(screen_w-110,screen_h));
  return dane;
}

void close_bot_d (void *data) 
{
    if(arena2)
    {
      destroy_bitmap(arena2);
      arena2=NULL;
    }
    free(data);
}
