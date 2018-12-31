/*----------------------------------------------------------------
 * core.c - core routines for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include <stdlib.h>

#include "libnet.h"
#include "internal.h"
#include "channels.h"
#include "conns.h"
#include "drivers.h"
#include "connhelp.h"
#include "classes.h"
#include "threads.h"
#include "timer.h"


static list_t detected_drivers;
static list_t initialised_drivers;
static list_t temp_detected_list;
list_t net_drivers_all;

static int initialised = 0;
static int done_atexit = 0;


/* exitfunc:  (internal)
 *  Calls net_shutdown.  net_init installs this as an atexit
 *  function.
 */
static void exitfunc (void) { net_shutdown(); }


/* net_init:
 *  Initialises the libnet library.
 */
int net_init (void) {
  if (!initialised) {
    initialised = 1;
    
    if (!__libnet_internal__mutex_create)
        net_set_mutex_funcs (NULL, NULL, NULL, NULL);
    if (!__libnet_timer_func)
        net_set_timer_func (NULL);
    __libnet_timer_func();
    
    detected_drivers = net_driverlist_create();
    initialised_drivers = net_driverlist_create();
    temp_detected_list = net_driverlist_create();
    net_drivers_all = net_driverlist_create();

    __libnet_internal__classes_init();
    __libnet_internal__drivers_init();
    __libnet_internal__channels_init();
    __libnet_internal__conns_init();
    
    if (!done_atexit) {
      if (atexit(exitfunc)) {
	exitfunc();
	return 1;
      }
      done_atexit = 1;
    } 
  }
  return 0;
}


static int detectdrivers_helper (int i, void *dat)
{
  NET_DRIVER *driver = __libnet_internal__get_driver (i);
  if (driver && (driver->detect())) {
    net_driverlist_add (detected_drivers, i);
    net_driverlist_add (temp_detected_list, i);
  }
  return 0;
}

/* net_detectdrivers:
 *  Attempts to detect each given driver; returns a list showing which
 *  of those specified was detected.
 */
list_t net_detectdrivers (list_t which) {
  net_driverlist_clear (temp_detected_list);
  net_driverlist_foreach (which, detectdrivers_helper, NULL);
  return temp_detected_list;
}


/* net_detectdriver:
 *  Attempts to detect a single driver; returns non-zero if detected.
 */
int net_detectdriver (int which) {
  list_t temp, detected;
  
  temp = net_driverlist_create();
  net_driverlist_clear(temp);
  net_driverlist_add (temp, which);
  detected = net_detectdrivers (temp);
  net_driverlist_destroy (temp);
  return net_driverlist_test (detected, which);
}


static int initdrivers_helper (int i, void *dat)
{
  NET_DRIVER *driver = __libnet_internal__get_driver (i);
  if (driver && (!driver->init()))
    net_driverlist_add (initialised_drivers, i);
  return 0;
}

/* net_initdrivers:
 *  Initialises the given drivers.  Returns the full list of
 *  successfully initialised drivers.
 */
list_t net_initdrivers (list_t which) {
  list_t temp1, temp2, temp3;
  temp1 = net_driverlist_create();
  temp2 = net_driverlist_create();
  temp3 = net_driverlist_create();

  /* Take a copy of our parameter, because the later call to 
   * `net_detectdrivers' could clobber it. */
  net_driverlist_add_list (temp3, which);
  which = temp3;
  
  /* Find undetected drivers */
  net_driverlist_add_list (temp2, which);
  net_driverlist_remove_list (temp2, detected_drivers);

  /* Try to detect them */
  net_detectdrivers (temp2);

  /* Mask out remaining undetected drivers */
  net_driverlist_add_list (temp2, which);
  net_driverlist_remove_list (temp2, detected_drivers);
  net_driverlist_add_list (temp1, which);
  net_driverlist_remove_list (temp1, temp2);
  
  /* Mask out already-initialised drivers */
  net_driverlist_remove_list (temp1, initialised_drivers);
  
  /* Do initialisation */
  net_driverlist_foreach (temp1, initdrivers_helper, NULL);
  
  net_driverlist_destroy (temp1);
  net_driverlist_destroy (temp2);
  net_driverlist_destroy (temp3);
  return initialised_drivers;
}


/* net_initdriver:
 *  Initialises a single driver.  Returns non-zero on success.
 */
int net_initdriver (int which) {
  list_t temp, inited;
  
  temp = net_driverlist_create();
  net_driverlist_clear(temp);
  net_driverlist_add (temp, which);
  inited = net_initdrivers (temp);
  net_driverlist_destroy (temp);
  return net_driverlist_test (inited, which);
}


static int exit_all_drivers_helper (int i, void *dat)
{
  NET_DRIVER *driver = __libnet_internal__get_driver (i);
  if (driver) driver->exit();
  return 0;
}

/* exit_all_drivers:  (internal)
 *  Shuts down all network drivers.
 */
static void exit_all_drivers (void) {
  net_driverlist_foreach (initialised_drivers, exit_all_drivers_helper, NULL);
  net_driverlist_clear (initialised_drivers);
}


/* net_shutdown:
 *  Shuts everything down nicely, returning 0 on success.
 */
int net_shutdown (void) {
  if (initialised) {
    __libnet_internal__conns_exit();
    __libnet_internal__channels_exit();
    exit_all_drivers();
    __libnet_internal__drivers_exit();
    __libnet_internal__classes_exit();
    
    net_driverlist_destroy (detected_drivers);
    net_driverlist_destroy (temp_detected_list);
    net_driverlist_destroy (initialised_drivers);
    net_driverlist_destroy (net_drivers_all);

    __libnet_internal__mutex_create = NULL;
    __libnet_timer_func = NULL;
    
    initialised = 0;
    return 0;
  } else return 1;
}


