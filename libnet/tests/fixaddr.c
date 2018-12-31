/* Test program for `fixupaddress' functions, mostly by Peter Wang */

#include <stdio.h>
#include <string.h>
#include <libnet.h>


#define DRIVER	NET_DRIVER_SOCKETS
#define ADDRESS	"127.0.0.1"


int main (int argc, char **argv)
{
    NET_CONN *listen, *conn = NULL;
    NET_CHANNEL *chan;
    char remote[NET_MAX_ADDRESS_LENGTH], buf[NET_MAX_ADDRESS_LENGTH];
    char *p, c;
    int server = -1;

    if (argc > 1) {
        if (!strcmp (argv[1], "server")) server = 1;
        else if (!strcmp (argv[1], "client")) server = 0;
    }
    if (server == -1) {
        puts ("Pass `server' or `client' on the command line.");
        return 1;
    }

    net_init();
    net_detectdrivers(net_drivers_all);
    net_initdrivers(net_drivers_all);

    if (server) {
        listen = net_openconn(DRIVER, "");
        net_listen(listen);
        while (!conn) 
            conn = net_poll_listen(listen);
    } else {
        conn = net_openconn(DRIVER, NULL);
        if (net_connect_wait_time(conn, ADDRESS, 5) != 0) {
            printf("Error connecting\n");
            return 1;
        }
    }

    chan = net_openchannel(DRIVER, NULL);
    p = net_getlocaladdress(chan);
    net_send_rdm(conn, p, strlen(p) + 1);

    while (!net_query_rdm(conn)) ;
    net_receive_rdm(conn, remote, sizeof remote);

    printf ("Address before fixing: %s\n", remote);

    if (net_fixupaddress_conn(conn, remote, buf) != 0) {
	printf("Didn't work\n");
	return 1;
    }

    printf ("Address after fixing: %s\n", buf);

    printf("assigning target: %s\n", buf);
    net_assigntarget(chan, buf);

    do {
	c = fgetc(stdin);
	if (c == '1') 
	    net_send(chan, "** Channel", 11);
	else if (c == '2')
	    net_send_rdm(conn, "** Conn", 8);

	while (net_query(chan)) {
	    net_receive(chan, buf, sizeof buf, NULL);
	    printf("%s\n", buf);
	}

	while (net_query_rdm(conn)) {
	    net_receive_rdm(conn, buf, sizeof buf);
	    printf("%s\n", buf);
	}
    } while (c != 'q');

    return 0;
}
