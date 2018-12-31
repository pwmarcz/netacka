/*----------------------------------------------------------------
 * dnstest.c - DNS test for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined TARGET_MSVC || defined TARGET_MINGW32
#include <windows.h>
#define sleep(x) Sleep ((x)*1000)
#else
#include <unistd.h>
#endif

#include <libnet.h>


void list_drivers (NET_DRIVERNAME *drvs, char *title) {
  printf ("\nDriver list: %s\n", title);
  while (drvs->name) {
    printf ("\t%d\t%s\n", drvs->num, drvs->name);
    drvs++;
  }
  printf ("\n");
}

int in_list (NET_DRIVERNAME *drvs, int x) {
  while (drvs->name) {
    if (x == drvs->num) return 1;
    drvs++;
  }
  return 0;
}

int main (void) {
  NET_DRIVERNAME *drivers;
  char buffer[NET_MAX_ADDRESS_LENGTH], buffer2[NET_MAX_ADDRESS_LENGTH];
  char *ch;
  int not_ok, x, choice;
  NET_DRIVERLIST avail;
  
  net_init();
  net_loadconfig (NULL);
  
  printf ("Detecting available drivers...\n");
  avail = net_detectdrivers (net_drivers_all);
  printf ("Getting detected driver names...\n");
  drivers = net_getdrivernames (avail);
  
  do {
    list_drivers (drivers, "Available drivers");
    
    printf ("Please choose a driver from the above list.\n");
    fgets (buffer, 10, stdin);
    choice = atoi (buffer);
  } while (!in_list (drivers, choice));
  free (drivers);
  
  avail = net_driverlist_create();
  net_driverlist_add (avail, choice);
  if (!net_initdrivers (avail)) {
    printf("Error initialising driver.\n");
    exit (1);
  }

  do {
    printf ("Enter address to prepare:\n");
    fgets (buffer, sizeof buffer, stdin);
    if (feof (stdin)) break;
    ch = strchr (buffer, '\n');
	if (!ch) {
      printf ("Too long for this lame program...\n");
      while (1) {
        char ch = getchar();
        if (ch == EOF || ch == '\n') break;
      }
    } else {
      NET_PREPADDR_DATA *data;
      *ch = '\0';
      printf ("Converting...\n");
      data = net_prepareaddress (choice, buffer, buffer2);
      if (!data) {
        printf ("Failed to initiate\n");
      } else {
        int x;
        printf ("Waiting...");
        do {
          printf (".");
          fflush(stdout);
          sleep (1);
          x = net_poll_prepareaddress (data);
        } while (!x);
        printf ("\n");
        if (x < 0) {
          printf ("Failed to convert address\n");
        } else {
          printf ("%s => %s\n", buffer, buffer2);
        }
      }
    }
  } while (!feof (stdin));

  printf ("Quitting...\n");
  
  return 0;
}

