/*----------------------------------------------------------------
 * serlinux.c - low-level Linux serial routines
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include "platdefs.h"

/* If we can use Linux serial ports, do so.  */
#ifdef __USE_REAL_SERIAL_LINUX__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/signal.h>
#include <sys/time.h>

#include "serial.h"


/* We could support more, but few people need it.  */
#define MAX_PORT	8


struct port {
    int fd;
    struct termios oldtio;
};

typedef struct port port_t;


static struct {
    int baudrate;
    int bits;
    int parity;
    int stopbits;
} config[MAX_PORT] = {
   { B115200, CS8, 0, 0 },
   { B115200, CS8, 0, 0 },
   { B115200, CS8, 0, 0 },
   { B115200, CS8, 0, 0 },
   { B115200, CS8, 0, 0 },
   { B115200, CS8, 0, 0 },
   { B115200, CS8, 0, 0 },
   { B115200, CS8, 0, 0 }
};


int __libnet_internal__serial_send (port_t *p,
				    const unsigned char *buf, int size)
{
    return write (p->fd, buf, size);
}


int __libnet_internal__serial_read (port_t *p, unsigned char *buf, int size)
{
    return read (p->fd, buf, size);
}


port_t *__libnet_internal__serial_open (int portnum)
{
    struct termios tio;
    char device[20];
    port_t *p;
    int fd;

    if ((portnum < 0) || (portnum >= MAX_PORT))
	return NULL;

    sprintf (device, "/dev/ttyS%d", portnum);
    fd = open (device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
	return NULL;

    p = malloc (sizeof *p);
    if (!p) {
	close (fd);
	return NULL;
    }

    p->fd = fd;
    tcgetattr (fd, &p->oldtio);

    memset (&tio, 0, sizeof tio);
    tio.c_cflag = (CLOCAL | CREAD
		   | config[portnum].baudrate 
		   | config[portnum].bits 
		   | config[portnum].parity
		   | config[portnum].stopbits);
		   /* CRTSCTS should be added if the DOS code 
		    * ever gets hardware flow control. */
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VTIME] = 0;
    tio.c_cc[VMIN] = 0;
    tcflush (fd, TCIOFLUSH);
    tcsetattr (fd, TCSANOW, &tio);

    return p;
}


void __libnet_internal__serial_close (port_t *p)
{
    tcdrain (p->fd);    
    tcsetattr (p->fd, TCSANOW, &p->oldtio);
    close (p->fd);
    free (p);
}


static int baudrate (int baud)
{
    switch (baud) {
	case 300:    return B300;
	case 600:    return B600;
	case 1200:   return B1200;
	case 2400:   return B2400;
	case 9600:   return B9600;
	case 19200:  return B19200;
	case 38400:  return B38400;
	case 57600:  return B57600;
	case 115200: return B115200;
	default:     return -1;
    }
}

static int bits (int bits)
{
    switch (bits) {
	case 5:  return CS5;
	case 6:  return CS6;
	case 7:  return CS7;
	case 8:  return CS8;
	default: return -1;
    }
}

static int stopbits (int bits)
{
    switch (bits) {
	case 1:  return 0;
	case 2:  return CSTOPB;
	default: return -1;
    }
}

static int parity (char *p) 
{
    if (!strcmp (p, "none")) return 0;
    if (!strcmp (p, "even")) return PARENB;
    if (!strcmp (p, "odd"))  return PARENB | PARODD;
    return -1;
}


void __libnet_internal__serial_load_config (int portnum, char *option,
					    char *value)
{
    #define SET(type, value)				\
	do {						\
	    int x = type (value);			\
	    if (x != -1) config[portnum].type = x;	\
	} while (0)
    
    if      (!strcmp (option, "baudrate")) SET (baudrate, atoi (value));
    else if (!strcmp (option, "bits"))	   SET (bits,	  atoi (value));
    else if (!strcmp (option, "stopbits")) SET (stopbits, atoi (value));
    else if (!strcmp (option, "parity"))   SET (parity,   value);
}


int __libnet_internal__serial_init (void) { return 0; }
int __libnet_internal__serial_exit (void) { return 0; }


#endif /* __USE_REAL_SERIAL_LINUX__ */
