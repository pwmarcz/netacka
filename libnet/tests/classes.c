#include <stdio.h>
#include <stdlib.h>

#include <libnet.h>

int main (void) 
{
	NET_DRIVERNAME *drivers;
	int i, j, class;
  	char buf[1024];

	net_init();
  
	puts ("Defined network classes:");

	for (j = 0; j < NET_CLASS_MAX; j++)
		if (net_classes[j].name)
			printf ("%d\t%s\n", j, net_classes[j].name);

	puts ("Choose one:");
	fgets (buf, sizeof buf, stdin);
	j = atoi (buf);
	printf ("\n\n%d: ", j);
	if (!net_classes[j].name) {
		puts ("no such class");
		return 1;
	}

	puts (net_classes[j].name);

	printf ("Address format information: ");
	if (!net_classes[j].addrhelp)
		puts ("None");
	else
		puts (net_classes[j].addrhelp);

	puts ("Drivers in this class:");

	drivers = net_getdrivernames (net_classes[j].drivers);

	for (i = 0; drivers[i].name; i++)
		printf ("%d\t%s\n", drivers[i].num, drivers[i].name);

	free (drivers);
	net_shutdown();

	return 0;
}
