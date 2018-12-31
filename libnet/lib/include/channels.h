/*----------------------------------------------------------------
 * channels.h - declarations for channel handlers (internal)
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */


#ifndef libnet_included_file_channels_h
#define libnet_included_file_channels_h


extern struct __channel_list_t {
  struct __channel_list_t *next;
  NET_CHANNEL *chan;
} *__libnet_internal__openchannels;


void __libnet_internal__channels_init (void);
void __libnet_internal__channels_exit (void);


#endif

