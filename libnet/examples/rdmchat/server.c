/*----------------------------------------------------------------
 * rdmchat/server.c - RDM chat server demo for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libnet.h>

#include "_coniox.h"
#include "chat.h"


#define SIZE 1000

/* Struct to hold the list of active connections */
typedef struct conn_list_t {
	struct conn_list_t *next;
	NET_CONN *conn;
	char *nick;
} conn_list_t;
conn_list_t *connlist = NULL;

/* One conn to listen for connections */
NET_CONN *listenconn;

char buffer[SIZE];
int netdriver = 0;


/* list_drivers, in_list, get_driver:
 *  These routines are used to get the user's driver choice.
 */
void list_drivers (NET_DRIVERNAME *drvs, char *title)
{
	printf("\nDriver list: %s\n",title);
	while (drvs->name) {
		printf("\t%d\t%s\n",drvs->num,drvs->name);
		drvs++;
	}
	printf ("\n");
}

int in_list (NET_DRIVERNAME *drvs, int x)
{
	while (drvs->name) {
		if (x==drvs->num) return 1;
		drvs++;
	}
	return 0;
}

void get_driver (void)
{
	char buffer[20];
	int choice;
	NET_DRIVERNAME *drivers;
	NET_DRIVERLIST avail;

	printf ("Detecting available drivers...\n");
	avail = net_detectdrivers (net_drivers_all);
	printf ("Getting detected driver names...\n");
	drivers = net_getdrivernames (avail);

	do {
		list_drivers (drivers, "Available drivers");

		printf ("Please choose a driver from the above list.\n");
		fgets (buffer,10,stdin);
		choice = atoi (buffer);
	} while (!in_list (drivers, choice));
	free (drivers);

	netdriver = choice;
}


/* init:
 *  Initialisation -- this initialises the driver and opens a conn
 *  to listening for connections.
 */
void init (void)
{
	NET_DRIVERLIST drv;
	drv = net_driverlist_create();
	net_driverlist_clear (drv);
	net_driverlist_add (drv, netdriver);

	if (!net_initdrivers (drv)) {
		printf("Error initialising driver.\n");
		exit (1);
	}
	net_driverlist_destroy (drv);

	listenconn = net_openconn (netdriver, "");
	if (!listenconn) {
		puts ("Couldn't open listening conn.");
		exit (2);
	}

	if (net_listen (listenconn)) {
		puts ("Failed to make conn listen.");
		exit (3);
	}

	connlist = NULL;
}


/* dispatch_message:
 *  Sends a message to all clients, and prints it on the console.
 */
void dispatch_message (char *message)
{
	conn_list_t *clist = connlist;

	while (clist) {
		net_send_rdm (clist->conn, message, strlen(message)+1);
		clist = clist->next;
	}
	conio_cputs (message);
	conio_cputs ("\n");
}


/* process_message:
 *  This routine takes a message from a client and acts upon it -- 
 *  normally it will distribute the message to all the clients, but
 *  messages starting with `/' are commands.
 */
void process_message (conn_list_t *clist)
{
	char *buf2;
	int x;

	strcpy (buffer, clist->nick);
	strcat (buffer, ": ");
	buf2 = strchr (buffer, '\0');
	x = net_receive_rdm (clist->conn, buf2, SIZE-1-(buf2-buffer));
	buf2[x] = '\0';

	if (buf2[0] == '/') {
		if (!strcmp (buf2+1,"quit")) {
			conn_list_t *p1 = NULL;

			if (clist != connlist)
				for (p1 = connlist; p1 && (p1->next != clist); p1 = p1->next);

			sprintf (buffer, "--- %s left", clist->nick);
			dispatch_message (buffer);

			strcpy (buffer, "*** go away");
			net_send_rdm (clist->conn, buffer, strlen (buffer));

			if (p1)
				p1->next = clist->next;
			else
				connlist = clist->next;

			net_closeconn (clist->conn);
			free (clist->nick);
			free (clist);
		} else
		if (!strncmp (buf2+1, "nick ", 5)) {
			char *oldnick = clist->nick;
			char *newnick = strdup (buf2+6);
			if (newnick) {
				clist->nick = newnick;
				sprintf (buffer, "=== %s changes nick to %s", oldnick, newnick);
				dispatch_message (buffer);
				free (oldnick);
			}
		}
	} else {
		dispatch_message (buffer);
	}
}

/* get_message:
 *  This routine scans the list of active clients for any who have sent
 *  a message, then sends the message on for further processing above.
 */
void get_message (void)
{
	conn_list_t *clist = connlist;
	int x=0;

	while (clist && !x) if (!(x=net_query_rdm (clist->conn))) clist = clist->next;
	if (x) process_message (clist);
}


/* check_for_new_connections:
 *  This looks to see whether any clients are trying to connect, and
 *  adds them to the client list if they are.
 */
void check_for_new_connections (void)
{
	NET_CONN *conn;
	int x;
	conn_list_t *clist;

	conn = net_poll_listen (listenconn);
	if (!conn) return;   /* no new connections */

	clist = malloc (sizeof *clist);
	if (clist) {
		clist->next = connlist;
		clist->conn = conn;
		clist->nick = strdup (net_getpeer (clist->conn));
		connlist = clist;
		sprintf (buffer, "+++ %s joins", clist->nick);
		dispatch_message (buffer);
	} else {
		sprintf (buffer, "!!! unable to add conn to list");
		dispatch_message (buffer);
	}
}


/* do_server:
 *  This is the main loop for the server.  It just alternates between
 *  checking for new clients trying to connect and checking for messages
 *  from existing clients.
 */
void do_server (void)
{
	conio_init();
	dispatch_message ("*** server started");
	do {
		check_for_new_connections();
		get_message();
	} while (!conio_kbhit());
	conio_getch();
	dispatch_message ("*** server shutting down");
	conio_exit();
}


int main (void)
{
	printf ("Libnet RDM chat server version %d.%d\n", VER_MAJOR, VER_MINOR);
	printf ("built at " __TIME__ " on " __DATE__ "\n");
	printf ("\n");

	net_init();
	net_loadconfig(NULL);
	get_driver();
	init();

	printf ("*** server awaiting connections\n");
	do_server();
	return 0;
}
END_OF_MAIN();

