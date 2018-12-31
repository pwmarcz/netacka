/*----------------------------------------------------------------
 * drivers.h - declarations for driver routines (internal)
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */


#ifndef libnet_included_file_drivers_h
#define libnet_included_file_drivers_h


#include <libnet.h>

int __libnet_internal__drivers_init (void);
void __libnet_internal__drivers_exit (void);
NET_DRIVER *__libnet_internal__get_driver (int type);
int __libnet_internal__get_driver_by_ptr (NET_DRIVER *ptr);


#endif
