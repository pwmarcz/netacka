#include <libnet.h>
#include <stdlib.h>
#include "net.h"

int net_driver;

int start_net()
{
   NET_DRIVERLIST drivers;
   NET_DRIVERNAME *drivernames;
   
   net_init();
   drivers = net_detectdrivers(net_classes[NET_CLASS_INET].drivers);
   drivernames = net_getdrivernames(drivers);
   net_driver = drivernames[0].num;
   free(drivernames);
   if(net_driver==-1) return -1;
   if(!net_initdriver(net_driver))
      return -1;
   return 0;
}


void send_byte(NET_CHANNEL *chan,unsigned char a)
{
   unsigned char data[1];
   
   data[0]=a;
   net_send(chan,data,1);
}

