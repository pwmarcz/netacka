#include <stdio.h>
#include <stdlib.h>

#include <libnet.h>

int main (void) 
{
  NET_DRIVERNAME *drivers;
  int i, class;
  
  net_init();
  
  printf ("Getting network driver name list...\n\n");
  
  drivers = net_getdrivernames (net_drivers_all);
  
  for (i = 0; drivers[i].name; i++) {
    printf ("%d\t%s", drivers[i].num, drivers[i].name);
    class = net_driver_class (drivers[i].num);
    printf ("  (class %d = %s)\n", class, net_classes[class].name);
  }

  free (drivers);
  net_shutdown();
  
  return 0;
}
