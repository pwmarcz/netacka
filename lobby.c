#include <stdlib.h>
#include <time.h>
#include <libnet.h>

#include "net.h"

#define MAX_SERVERS 16

struct {
   char addr[50];
   int active;
   int next_report;
} servers[MAX_SERVERS];
int n_active=0;

void send_servers(NET_CHANNEL *chan,int n)
{
   int i,j;
   char data[54];
   
   if(n_active<n) n=n_active;
   for(i=j=0;j<n;j++)
   {
      int len;
      
      while(!servers[i].active) i++;

      len=strlen(servers[i].addr)+1;
      data[0]=loASERVER;
      data[1]=len;
      strcpy(&data[2],servers[i].addr);
      net_send(chan,data,len+4);
      printf("Gave him %s\n",&data[2]);
      i++;
   }
}

int main(int argc,char *argv[])
{
   NET_CHANNEL *chan;
   char binding[50];
   time_t lastping;

   if(argc<2)
   {
      strcpy(binding,"0.0.0.0:6790");
   }
   else
   {
      strcpy(binding,argv[1]);
   }
   if(start_net()!=0) return 1;
   if(chan=net_openchannel(net_driver,binding))
   {
      int i;
      printf("Opened on %s\n",binding);
      for(i=0;i<MAX_SERVERS;i++)
         servers[i].active=0;
      lastping=time(0);
      for(;;)
      {
         sleep(1);
         {
            int i;
            for(i=0;i<MAX_SERVERS;i++)
            {
               if(!servers[i].active)
                  continue;
               if(servers[i].next_report<time(0))
               {
                  printf("Deleted %s\n",servers[i].addr);
                  servers[i].active=0;
                  n_active--;
               }
            }
         }
         while(net_query(chan))
         {
            char data[2];
            char from[50];
            int n;
            int j;
            
            n=net_receive(chan,data,2,from);
            for(i=0;i<MAX_SERVERS;i++)
            {
               if(servers[i].active)
                  if(!strcmp(from,servers[i].addr))
                     break;
            }
            switch(*data)
            {
               case seREGISTERME:
                  if(i==MAX_SERVERS)
                  {
                     if(n_active<MAX_SERVERS)
                     {
                        printf("Registered %s\n",from);
                        for(i=0;servers[i].active;i++);
                        servers[i].active=1;
                        servers[i].next_report=time(0)+30;
                        strcpy(servers[i].addr,from);
                        n_active++;
                     }
                  }
                  else
                     servers[i].next_report=time(0)+30;
                  break;
               case seIMOFF:
                  if(i<MAX_SERVERS)
                  {
                     printf("Deleted %s\n",from);
                     servers[i].active=0;
                     n_active--;
                  }
                  break;
               case clGIMMESERVERS:
                  printf("%s wanted to know servers\n",from);
                  net_assigntarget(chan,from);
                  send_servers(chan,data[1]);
                  break;
               default:
                  break;
            }
         }
      }
   }
   return 0;
}
