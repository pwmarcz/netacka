/*----------------------------------------------------------------
 * rdmtest.c - RDM test program for Libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

/* Based on gentest. */


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




#define SIZE 70

/* list_drivers:
 *  Displays a list of the drivers in the given list.
 */
void list_drivers (NET_DRIVERNAME *drvs, char *title) {
  printf ("\nDriver list: %s\n", title);
  while (drvs->name) {
    printf ("\t%d\t%s\n", drvs->num, drvs->name);
    drvs++;
  }
  printf ("\n");
}

/* in_list:
 *  Checks whether driver `x' is in list `drvs'.
 */
int in_list (NET_DRIVERNAME *drvs, int x) {
  while (drvs->name) {
    if (x == drvs->num) return 1;
    drvs++;
  }
  return 0;
}

/* get_and_init_driver:
 *  Gets some driver lists from the library, asks the user to choose a driver,
 *  and then initialises that driver.
 */
int get_and_init_driver (void)
{
  NET_DRIVERNAME *drivers;
  NET_DRIVERLIST avail;
  int choice;
  char buffer[16];

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

  return choice;
}

/* server_startup:
 *  Startup routine for the server.  It opens a conn, listens for connections,
 *  and opens another conn when it gets a connection.
 */
void server_startup (int driver, NET_CONN **conn, NET_CONN **conn2)
{
  if (!(*conn2 = net_openconn (driver, ""))) {
    printf("Couldn't open conn.\n");
    exit (2);
  }
  
  printf ("Listening");
  fflush (stdout);
  if (!net_listen (*conn2)) {
    do {
      putchar ('.');
      fflush (stdout);
      sleep (1);
      *conn = net_poll_listen (*conn2);
    } while (!*conn);
    if (!*conn) {
      puts ("User aborted.");
      exit (3);
    }
  } else {
    puts ("Error occured.");
    exit (3);
  }
}

/* callback used by `client_startup' below */
int callback (void)
{
	putchar ('.');
	fflush (stdout);
	return 0;
}

/* client_startup:
 *  Startup routine for the client.  It opens a conn and connects it to the
 *  server.
 */
void client_startup (int driver, NET_CONN **conn, char *target)
{
  int x;

  if (!(*conn = net_openconn (driver, NULL))) {
    printf("Couldn't open conn.\n");
    exit (2);
  }
  
  printf ("Connecting");
  fflush (stdout);
  x = net_connect_wait_cb (*conn, target, callback);
  if (x) {
    if (x > 0)
      puts ("User aborted.");
    else
      puts ("Error occured.");
    net_closeconn (*conn);
    exit(3);
  }
}

  
int main (void) {
  NET_CONN *conn, *conn2 = NULL;
  char buffer[SIZE]; 
  char target[1024];
  int client, driver;
  
  net_init();
  net_loadconfig (NULL);
  
  driver = get_and_init_driver();
  
  puts ("Enter target address (blank to be server):\n");
  fgets (target, 1024, stdin);
  { char *ch = strchr (target, '\n'); if (ch) *ch = '\0'; }
  
  client = !!target[0];
  
  /* This program is basically peer-to-peer; the only difference between client
   * and server is in the connection protocol.  So here we call the appropriate
   * connection routine; after this, the client and server are connected and
   * both use the same code to communicate.
   */
  if (!client)
    server_startup (driver, &conn, &conn2);
  else
    client_startup (driver, &conn, target);

  puts   ("Connection established.\n");
  
  puts   ("Now type lines of text, which will be sent through the connection.");
  puts   ("Incoming data will be checked for after each line.");
  puts   ("Type a line empty except for a full stop to quit.\n");

  printf ("The maximum line length is just over %d characters.\n\n", SIZE-5);
  
  do {
    /* If we're serving, check whether the client is still trying to connect.
     * This might seem a bit odd; it's here in case our acknowledgement of
     * their attempt does not get through.  The library will resend the ack if
     * the same client tries to connect again; we don't need to catch the
     * returned conn because we don't really care what it is.
     * 
     * If we were planning to serve to more than one client, with clients able
     * to connect at any time, we'd have a different system here; see the
     * rdmchat example program.
     */
    if (!client) net_poll_listen (conn2);
    
    /* This is the receive loop.  We check whether messages are waiting, and if
     * there are any we process them.  We are wide open to flooding here -- in
     * practice it might be better to restrict the number of messages we
     * process each cycle.
     */
    while (net_query_rdm (conn)) {
      int x;
      x = net_receive_rdm (conn, buffer, SIZE - 1);
      if (x == -1) {
	puts ("[recv failure]");
      }	else {
	buffer[x] = 0;
	printf ("recv: %s\n", buffer);
      }
    }
    
    /* This demonstrates the net_conn_stats function -- it gets the number of
     * incoming packets waiting to be read and the number of unacknowledged
     * outgoing packets.  Probably not particularly useful in practice, but you
     * might be able to optimise your networking system using this information.
     */
    {
      int in_q, out_q;
      net_conn_stats (conn, &in_q, &out_q);
      printf ("(%d:%d) ", in_q, out_q);
    }
    
    /* Get input and send it.  This blocks, of course, so you won't see
     * incoming data until you hit Enter.
     */
    {
      char *chptr;
      printf ("send: ");
      fgets (buffer, SIZE - 5, stdin);
      chptr = strchr (buffer, '\0');
      if (*(chptr-1) == '\n') *--chptr = 0;
      if (chptr != buffer) {
	int x;
        x = net_send_rdm (conn, buffer, chptr - buffer);
        if (x) puts ("[send failure]");
      }
    }
  } while (strcmp (buffer,"."));
  
  puts ("Quitting...");
  net_closeconn (conn);
  if (!client) net_closeconn (conn2);
  
  return 0;
}

