/*----------------------------------------------------------------
 *   classes.c -- network classes
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include <libnet.h>

#include "classes.h"


NET_CLASS net_classes[NET_CLASS_MAX] = {
	{ "None",     NULL, 0 },
	{ "Internet", NULL, 0 },
	{ "IPX",      NULL, 0 },
	{ "Serial",   NULL, 0 },
	{ "Local",    NULL, 0 }
};

void __libnet_internal__classes_init (void)
{
	int i;
	for (i = 0; i < NET_CLASS_MAX; i++)
		if (net_classes[i].name)
			net_classes[i].drivers = net_driverlist_create();
}

void __libnet_internal__classes_exit (void)
{
	int i;
	for (i = 0; i < NET_CLASS_MAX; i++)
		if (net_classes[i].name)
			net_driverlist_destroy (net_classes[i].drivers);
}

