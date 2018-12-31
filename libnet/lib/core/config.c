/*----------------------------------------------------------------
 * config.c - configuration file routines for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef TARGET_MSVC
#include <unistd.h>
#else
#define S_ISDIR(x) (x & _S_IFDIR)
#endif

#include "libnet.h"
#include "internal.h"
#include "drivers.h"
#include "config.h"

static char default_config_filename[] = "libnet.cfg";

static int loadconfig_helper (int id, void *config_fp)
{
  NET_DRIVER *drv = __libnet_internal__get_driver (id);
  if (!drv) return 0;
  if (drv->load_conn_config) drv->load_conn_config (drv, config_fp, "conns");
  drv->load_config (drv, config_fp);
  return 0;
}

/* net_loadconfig:
 *  Loads a config file for the network drivers.  Pass a filename, 
 *  directory or NULL.  If you pass only a filename it is assumed to be
 *  in the current directory.  If you pass a directory, the filename is
 *  taken to be default_config_filename above.  If you pass NULL, this
 *  loads deafult_config_filename from the program's directory.  Returns 
 *  zero on success.
 */
int net_loadconfig (const char *fname) {
  struct stat st;
  char filename[1024];
  FILE *config_fp;
  
  if (fname == NULL)
    strcpy (filename, default_config_filename);
  else {
    strcpy (filename, fname);
    if (stat (filename, &st)) return 1;                           /* file not found? */
    if (S_ISDIR (st.st_mode)) {					/* directory */
      strcat (filename, "/");
      strcat (filename, default_config_filename);
    }
  }
  
  config_fp = fopen (filename, "rt");
  if (!config_fp) return 2;              /* file opening error */
  
  net_driverlist_foreach (net_drivers_all, loadconfig_helper, config_fp);
  
  fclose (config_fp);
  return 0;                       /* success */
}

/* __libnet_internal__seek_section:
 *  This function is used internally; it searches the given open text 
 *  file `fp' for a line containing only the string `section' inside
 *  square brackets.  Returns zero on success.
 */
int __libnet_internal__seek_section (FILE *fp, const char *section) {
  char buffer [1024];
  char target [1024];
  
  strcpy (target, "[");
  strcat (target, section);
  strcat (target, "]\n");
  
  rewind (fp);
  do {
    fgets (buffer, 1024, fp);
    if (strlen (buffer) == 1023)
      while ((strlen (buffer) == 1023) && fgets (buffer, 1024, fp));
    else {
      if (!strcmp (buffer, target)) return 0;
    }
  } while (!feof (fp));
  
  return 1;
}

/* __libnet_internal__get_setting:
 *  This internal function is used to read the first "option = value"
 *  pair from `fp'.  It strips leading and trailing whitespace from
 *  both `*option' and `*value', and stops looking if it finds a line
 *  starting with `['.  Returns zero if it found a pair.
 */
int __libnet_internal__get_setting (FILE *fp, char **option, char **value) {
  static char buffer [1024];
  char *chp;
  
  if ((!option) || (!value) || (!fp)) return 1;
  
  do {
    fgets (buffer, 1024, fp);
    if (strlen (buffer) == 1023) {
      while ((strlen (buffer) == 1023) && fgets (buffer, 1024, fp));
      buffer [0] = 0;
    }
  } while ((!feof (fp)) && ((buffer[0] == '#') || ((buffer[0] != '[') && (!strchr (buffer, '=')))));
  
  if ((buffer[0] != '#') && (chp = strchr (buffer, '='))) {
    *option = buffer;
    *value = chp + 1;
    *chp = 0;
    
    while (isspace ((unsigned)**value)) (*value)++;
    chp = strchr (*value, 0) - 1;
    while ((chp >= *value) && isspace ((unsigned)*chp)) *chp-- = 0;
    
    while (isspace ((unsigned)**option)) (*option)++;
    chp = strchr (*option, 0) - 1;
    while ((chp >= *option) && isspace ((unsigned)*chp)) *chp-- = 0;
    
    return 0;
  }
  
  return 1;
}
