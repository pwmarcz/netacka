/*----------------------------------------------------------------
 * config.h - configuration file routines for libnet (internal)
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */


#ifndef libnet_included_file_config_h
#define libnet_included_file_config_h


int __libnet_internal__seek_section (FILE *fp, const char *section);
int __libnet_internal__get_setting (FILE *fp, char **option, char **value);


#endif
