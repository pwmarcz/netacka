/*----------------------------------------------------------------
 * conns.h - declarations for conn handlers (internal)
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */


#ifndef libnet_included_file_conns_h
#define libnet_included_file_conns_h


extern struct __conn_list_t {
  struct __conn_list_t *next;
  NET_CONN *conn;
} *__libnet_internal__openconns;


void __libnet_internal__conns_init (void);
void __libnet_internal__conns_exit (void);


#endif

