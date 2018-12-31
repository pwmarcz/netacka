/*----------------------------------------------------------------
 * chat/server.c - chat server demo for libnet
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


typedef struct chan_list_t {
	struct chan_list_t *next;
	NET_CHANNEL *chan;
	char *nick;
} chan_list_t;

chan_list_t *channellist = NULL;
NET_CHANNEL *listenchan;
char buffer[SIZE];
int netdriver = 0;


void list_drivers (NET_DRIVERNAME *drvs, char *title) {
	printf("\nDriver list: %s\n",title);
	while (drvs->name) {
		printf("\t%d\t%s\n",drvs->num,drvs->name);
		drvs++;
	}
	printf ("\n");
}


int in_list (NET_DRIVERNAME *drvs, int x) {
	while (drvs->name) {
		if (x==drvs->num) return 1;
		drvs++;
	}
	return 0;
}


void get_driver() {
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


void init() {
	NET_DRIVERLIST drv;
	drv = net_driverlist_create();
	net_driverlist_clear (drv);
	net_driverlist_add (drv, netdriver);

	if (!net_initdrivers (drv)) {
		printf("Error initialising driver.\n");
		exit (1);
	}
	net_driverlist_destroy (drv);

	listenchan = net_openchannel (netdriver, "");
	if (!listenchan) {
		printf ("Couldn't open input channel.\n");
		exit (2);
	}

	channellist = NULL;
}


void dispatch_message (char *message) {
	chan_list_t *chanlist = channellist;

	while (chanlist) {
		net_send (chanlist->chan, message, strlen(message)+1);
		chanlist = chanlist->next;
	}
	conio_cputs (message);
	conio_cputs ("\n");
}

/* 
 * Returns internal data structure with client's data based on their channel,
 * using a linear search (Jason Winnebeck), returns NULL on error.
 */
chan_list_t *discover_client(void *chan) {
	chan_list_t *ret = channellist;
	while (ret) {
		if (ret->chan == chan)
			return ret;
		ret = ret->next;
	}
	return NULL;
}

char *get_message(chan_list_t *chanlist) {
	char *buf2;
	int x = 0;

	if (chanlist == NULL) {
		sprintf(buffer, "!!! unregistered client");
		return NULL;
	}

	strcpy (buffer, chanlist->nick);
	strcat (buffer, ": ");
	buf2 = strchr (buffer, 0);
	x = net_receive (chanlist->chan, buf2, SIZE-1-(buf2-buffer), NULL);
	buf2[x] = 0;
		
	if (buf2[0] == '/') {  
		if (!strcmp (buf2+1,"quit")) {
			chan_list_t *p1 = NULL;
			
			if (chanlist != channellist)
				for (p1 = channellist; p1 && (p1->next != chanlist); p1 = p1->next);
			
			sprintf (buffer, "--- %s left", chanlist->nick);
			dispatch_message (buffer);
			
			strcpy (buffer, "*** go away");
			net_send (chanlist->chan, buffer, strlen (buffer));
			
			if (p1) p1->next = chanlist->next; else channellist = chanlist->next;
			net_closechannel (chanlist->chan);
			free (chanlist->nick);
			free (chanlist);
		}
		
		return NULL;
	} else 
		return buffer;
}


void check_for_new_connections() {
	char address[32], *newnick;
	int x = net_receive (listenchan, buffer, SIZE-1, address);

	if (x && (buffer[0] == CHAT_MAGIC)) {
		chan_list_t *chanlist = (chan_list_t *) malloc (sizeof (chan_list_t));
		buffer[x] = 0;
		if (!(chanlist->chan = net_openchannel (netdriver, NULL))) {
			sprintf (buffer, "!!! failed to open channel for %s",address);
			dispatch_message (buffer);
			free (chanlist);
			return;
		}
		if (net_assigntarget (chanlist->chan, address)) {
			sprintf (buffer, "!!! couldn't assign target `%s' to channel",address);
			dispatch_message (buffer);
			net_closechannel (chanlist->chan);
			free (chanlist);
			return;
		}

		newnick = strdup (buffer+1);
		sprintf (buffer, "+++ %s joined from %s", newnick, address);
		dispatch_message (buffer);
		strcpy (buffer, "OK");
		net_send (chanlist->chan, buffer, strlen (buffer));
		chanlist->nick = newnick;
		chanlist->next = channellist;
		channellist = chanlist;
	}
}


void do_server() {
	char *message = NULL;
	conio_init();
	dispatch_message ("*** server started");
	do {
		void *next = net_wait_all(1000);
		if (next != NULL) {
			if (next == listenchan) {
				check_for_new_connections();
			} else {
				message = get_message(discover_client(next));
				if (message) dispatch_message (message);
			}
		}
	} while (!conio_kbhit());
	conio_getch();
	sprintf (buffer, "*** server shutting down");
	dispatch_message (buffer);
	conio_exit();
}


int main(void) {
	printf ("libnet chat server version %d.%d\n", VER_MAJOR, VER_MINOR);
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
