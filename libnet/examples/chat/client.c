/*----------------------------------------------------------------
 * chat/client.c - chat client demo for libnet
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


NET_CHANNEL *chan;
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
	char temp[1024], nick[1024], addr[1024], newaddr[NET_MAX_ADDRESS_LENGTH];
	NET_DRIVERLIST drv;
	drv = net_driverlist_create();
	net_driverlist_clear (drv);
	net_driverlist_add (drv, netdriver);

	if (!net_initdrivers (drv)) {
		printf("Error initialising driver.\n");
		exit (1);
	}

	printf ("Enter target address: ");
	fgets (addr, 1024, stdin);
	while (strchr(addr,'\n')) *strchr(addr,'\n')=0;

	printf ("Enter nickname: ");
	fgets (nick, 10, stdin);
	while (strchr(nick,'\n')) *strchr(nick,'\n')=0;

	if (!(chan = net_openchannel (netdriver, NULL))) {
		printf ("Unable to open channel.\n");
		exit (2);
	}

	printf ("Connecting to %s...\n", addr);

	net_assigntarget (chan, addr);
	sprintf (temp, "%c%s", CHAT_MAGIC, nick);
	net_send (chan, temp, strlen (temp));

	while ((!net_query (chan))/* && !conio_kbhit()*/);

	if (0/*conio_kbhit()*/) {
		conio_getch();
		printf ("Aborted.\n");
		exit (3);
	}

	{
		int x = net_receive (chan, temp, 1024, newaddr);
		if (x == -1) {
			printf ("Receive error.\n");
			exit (5);
		}
		temp[x] = 0;
	}

	if (strcmp (temp, "OK")) {
		printf ("Connection refused.\n");
		exit (4);
	}

	printf ("Connection accepted, redirecting... ");
	fflush (stdout);
	net_assigntarget (chan, newaddr);
	printf ("done.\n");
}


void erase_buffer() {
	conio_gotoxy (1,25);
	conio_clreol();
}


void show_buffer(char *buffer, int pos) {
	char *start = buffer+pos-75;
	if (start<buffer) start = buffer;
	conio_gotoxy (1,25);
	conio_textcolor (14);
	conio_cputs (start);
	conio_clreol();
}


void write_to_window (char *buf, int col) {
	erase_buffer();
	conio_gotoxy (1,24);
	conio_textcolor (col);
	conio_cputs (buf);
	conio_cputs ("\n");
	conio_cputs ("\n");
}


void do_client() {
	char obuffer[1024] = {0}, ibuffer[1024] = {0};
	int obuffer_offset = 0, stop = 0;

	fflush (stdout);  /* get anything out of the buffer */
	conio_init();
	show_buffer (obuffer, obuffer_offset);

	do {

		if (net_query (chan)) {
			int col = 10;
			int x = net_receive (chan, ibuffer, 1024, NULL);

			if (x<0)
				strcpy (ibuffer, "!!! (local) error reading packet");
			else
				ibuffer[x] = 0;

			switch (ibuffer[0]) {
				case '*': col = 9; break;
				case '+':
				case '-': col = 11; break;
				case '!': col = 12; break;
			}

			write_to_window (ibuffer, col);
			show_buffer (obuffer, obuffer_offset);

			if (!strcmp (ibuffer, "*** go away")) stop = 1;
			if (!strcmp (ibuffer, "*** server shutting down")) stop = 1;
		}

		if (conio_kbhit()) {
			char ch = conio_getch();
			switch (ch) {
			    	case 7:
				case 8:
					if (obuffer_offset) obuffer[--obuffer_offset] = 0;
					show_buffer (obuffer, obuffer_offset);
					break;
				case 13:
					net_send (chan, obuffer, strlen (obuffer));
					obuffer[obuffer_offset = 0] = 0;
					show_buffer (obuffer, obuffer_offset);
					break;
				default:
					obuffer[obuffer_offset] = ch;
					obuffer[++obuffer_offset] = 0;
					show_buffer (obuffer, obuffer_offset);
					break;
			}
		}

	} while (!stop);

	erase_buffer();
	conio_exit();
}


int main(void) {
	printf ("libnet chat client version %d.%d\n", VER_MAJOR, VER_MINOR);
	printf ("built at " __TIME__ " on " __DATE__ "\n");
	printf ("\n");

	net_init();
	net_loadconfig (NULL);

	get_driver();
	init();

	do_client();

	return 0;
}
END_OF_MAIN();

