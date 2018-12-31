/*----------------------------------------------------------------
 * drivers.c -- network driver control functions
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include <stdlib.h>

#include <libnet.h>

#include "drivers.h"
#include "internal.h"
#include "connhelp.h"


extern NET_DRIVER
  net_driver_nonet,
  net_driver_local,
  net_driver_wsock_dos,
  net_driver_wsock_win,
  net_driver_sockets,
  net_driver_ipx_dos,
  net_driver_ipx_win,
  net_driver_ipx_linux,
  net_driver_serial_dos,
  net_driver_serial_linux,
  net_driver_serial_beos,
  net_driver_besocks;

struct driver_table_entry {
  int num;
  NET_DRIVER *driver;
  struct driver_table_entry *next;
};

static struct driver_table_entry *driver_table;


/* __libnet_internal__get_driver:  (internal)
 *  Returns a pointer to the VFT for the given driver.  NULL if it
 *  doesn't exist.
 */
NET_DRIVER *__libnet_internal__get_driver (int num) {
  struct driver_table_entry *drv = driver_table;
  while (drv && (drv->num != num))
    drv = drv->next;
  if (drv)
    return drv->driver;
  else
    return NULL;
}

/* __libnet_internal__get_driver_by_ptr:  (internal)
 *  Returns the ID number of the given driver, or -1 if it doesn't exist.
 */
int __libnet_internal__get_driver_by_ptr (NET_DRIVER *ptr) {
  struct driver_table_entry *drv = driver_table;
  while (drv && (drv->driver != ptr))
    drv = drv->next;
  if (drv)
    return drv->num;
  else
    return -1;
}


static void check_conn_funcs (NET_DRIVER *driver)
{
	if (driver->init_conn) return;
	#define cp(x) driver->x = __libnet_internal__wrapper_driver.x
	cp (init_conn);
	cp (destroy_conn);
	cp (listen);
	cp (poll_listen);
	cp (connect);
	cp (poll_connect);
	cp (send_rdm);
	cp (recv_rdm);
	cp (query_rdm);
	cp (conn_stats);
	cp (load_conn_config);
	#undef cp
}


int net_register_driver (int num, NET_DRIVER *driver)
{
  struct driver_table_entry *p;

  check_conn_funcs (driver);

  if (num == 0) {
    num = NET_DRIVER_USER / 2;
    while (net_driverlist_test (net_drivers_all, num) && (num < NET_DRIVER_USER))
      num++;
    if (num == NET_DRIVER_USER) return 0;
  }
 
  if (!net_driverlist_add (net_drivers_all, num)) return 0;
  if (!net_driverlist_add (net_classes[driver->class].drivers, num)) {
    net_driverlist_remove (net_drivers_all, num);
    return 0;
  }

  p = malloc (sizeof (*p));
  if (!p) {
    net_driverlist_remove (net_drivers_all, num);
    net_driverlist_remove (net_classes[driver->class].drivers, num);
    return 0;
  }

  p->num = num;
  p->driver = driver;
  p->next = driver_table;
  driver_table = p;

  return num;
}

int __libnet_internal__drivers_init (void)
{
    driver_table = NULL;
    net_register_driver (NET_DRIVER_NONET,        &net_driver_nonet);
    net_register_driver (NET_DRIVER_LOCAL,        &net_driver_local);
    net_register_driver (NET_DRIVER_WSOCK_DOS,    &net_driver_wsock_dos);
    net_register_driver (NET_DRIVER_WSOCK_WIN,    &net_driver_wsock_win);
    net_register_driver (NET_DRIVER_SOCKETS,      &net_driver_sockets);
    net_register_driver (NET_DRIVER_IPX_DOS,      &net_driver_ipx_dos);
    net_register_driver (NET_DRIVER_IPX_WIN,      &net_driver_ipx_win);
    net_register_driver (NET_DRIVER_IPX_LINUX,    &net_driver_ipx_linux);
    net_register_driver (NET_DRIVER_SERIAL_DOS,   &net_driver_serial_dos);
    net_register_driver (NET_DRIVER_SERIAL_LINUX, &net_driver_serial_linux);
    net_register_driver (NET_DRIVER_SERIAL_BEOS,  &net_driver_serial_beos);
    net_register_driver (NET_DRIVER_BESOCKS,      &net_driver_besocks);
    return 0;
}

void __libnet_internal__drivers_exit (void)
{
    while (driver_table) {
      struct driver_table_entry *p = driver_table->next;
      free (driver_table);
      driver_table = p;
    }
}

/* net_driver_class:
 *  Returns the class ID of the given driver (by ID).
 */
int net_driver_class (int drvid)
{
	NET_DRIVER *drv = __libnet_internal__get_driver (drvid);
	if (!drv) return -1;
	return drv->class;
}


static int getdrivernames_helper (int i, void *dat)
{
  NET_DRIVERNAME **temp_ptr_ptr = dat;
  (*temp_ptr_ptr)->num = i;
  (*temp_ptr_ptr)->name = __libnet_internal__get_driver(i)->name;
  (*temp_ptr_ptr)++;
  return 0;
}

/* net_getdrivernames:
 *  Returns a list of driver names; the list contains one entry for 
 *  each driver queried in `which', followed by a terminating entry 
 *  which has a NULL driver name.  The list should be freed by the
 *  caller (using `free').
 */
NET_DRIVERNAME *net_getdrivernames (list_t which) {
  NET_DRIVERNAME *retval, *temp_ptr;
  int num;
  
  num = net_driverlist_count (which);
  retval = (NET_DRIVERNAME *) malloc (sizeof (NET_DRIVERNAME) * (num + 1));

  temp_ptr = retval;
  net_driverlist_foreach (which, getdrivernames_helper, &temp_ptr);
  
  temp_ptr->num = -1;
  temp_ptr->name = NULL;
  
  return retval;
}

