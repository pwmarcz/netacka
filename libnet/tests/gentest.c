/*----------------------------------------------------------------
 * gentest.c - Generic test for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libnet.h>


#define SIZE 70


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

void check_config() {
  FILE *fp;
  char buffer[1000];
  
  fp = fopen ("libnet.cfg", "rt");
  if (!fp) {
    fprintf (stderr, "Can't find config file; never mind...\n");
    return;
  }
  
  fgets (buffer, 1000, fp);
  if (!strcmp (buffer, "# CHANGE ME!\n")) {
    fprintf (stderr, "You haven't edited the config file.\nPlease do so (instructions within).\n");
    exit (4);
  }
  fclose (fp);
}

int main (void) {
  NET_DRIVERNAME *drivers;
  NET_CHANNEL *chan;
  char buffer[SIZE]; 
  char target[1024];
  char *chptr;
  int not_ok, x, choice;
  NET_DRIVERLIST avail;
  
  check_config();
  
  net_init();
  net_loadconfig (NULL);
  
  printf ("Getting driver names...\n");
  drivers = net_getdrivernames (net_drivers_all);
  
  list_drivers (drivers, "All drivers");
  free (drivers);
  
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
  
  if (!(chan = net_openchannel (choice, NULL))) {
    printf("Couldn't open network channel.\n");
    exit (2);
  }
  
  printf("\nReturn address is: %s\n", net_getlocaladdress (chan));
  
  do {
    printf("Enter target address (blank to quit):\n");
    fgets(target,1024,stdin);
    
    if (target[1]) {
      printf ("Checking address...\n");
      not_ok = net_assigntarget (chan,target);
      if (not_ok) printf("\nCouldn't use that address; please try another.\n");
    } else not_ok=0;
  } while (not_ok);
  
  if (target[1]==0) {
    printf("Aborted.\n");
    exit(3);
  }
  
  printf("\nNow type lines of text, which will be sent through the connection.\n");
  printf("Incoming data will be checked for after each line.\n");
  printf("Type a line empty except for a full stop to quit.\n\n");
  printf("The maximum line length is just over %d characters.\n\n",SIZE-5);
  
  do {
    while (net_query (chan)) {
      x = net_receive (chan, buffer, SIZE - 1, NULL);
      if (x == -1) {
	puts ("[recv failure]");
      }	else {
	buffer[x] = 0;
	printf ("recv: %s\n", buffer);
      }
    }
    printf ("send: ");
    fgets (buffer, SIZE - 5, stdin);
    chptr = strchr (buffer, '\0');
    if (*(chptr-1) == '\n') *--chptr = 0;
    if (chptr != buffer) {
      x = net_send (chan, buffer, chptr - buffer);
      if (x) puts ("[send failure]");
    }
  } while (strcmp (buffer,"."));
  
  printf ("Quitting...\n");
  
  return 0;
}
