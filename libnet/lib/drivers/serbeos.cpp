/*----------------------------------------------------------------
 * serbeos.c - low-level BeOS serial routines
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include "platdefs.h"

/* If we can use BeOS serial ports, do so. */
#ifdef __USE_REAL_SERIAL_BEOS__

#include <SerialPort.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "serial.h"
}


#define CFUNC	extern "C" 


#define MAX_PORT    4


struct port {
    BSerialPort *serial;
};


static struct {
    data_rate baudrate;
    data_bits bits;
    parity_mode parity;
    stop_bits stopbits;
} config[] = {
    { B_115200_BPS, B_DATA_BITS_8, B_NO_PARITY, B_STOP_BITS_1 },
    { B_115200_BPS, B_DATA_BITS_8, B_NO_PARITY, B_STOP_BITS_1 },
    { B_115200_BPS, B_DATA_BITS_8, B_NO_PARITY, B_STOP_BITS_1 },
    { B_115200_BPS, B_DATA_BITS_8, B_NO_PARITY, B_STOP_BITS_1 }
};


CFUNC struct port *__libnet_internal__serial_open(int portnum)
{
    BSerialPort *serial = NULL;
    struct port *port = NULL;
    char device[B_OS_NAME_LENGTH];

    if ((portnum < 0) || (portnum >= MAX_PORT))
	return NULL;

    serial = new BSerialPort;
    serial->GetDeviceName(portnum, device);
    if (serial->Open(device) <= 0)
	goto cleanup;
    
    serial->SetDataRate(config[portnum].baudrate);
    serial->SetDataBits(config[portnum].bits);
    serial->SetStopBits(config[portnum].stopbits);
    serial->SetParityMode(config[portnum].parity);
    serial->SetBlocking(false);
    serial->SetFlowControl(0);
    
    port = (struct port *)malloc(sizeof *port);
    if (!port) goto cleanup;
    port->serial = serial;
    return port;

  cleanup:

    if (serial) {
	serial->Close();
	delete serial;
    }
    
    if (port) free(port);

    return NULL;
}


CFUNC void __libnet_internal__serial_close(struct port *p)
{
    p->serial->Close();
    delete p->serial;
    free(p);
}


CFUNC int __libnet_internal__serial_send(struct port *p, const unsigned char *buf, int size)
{
    return p->serial->Write(buf, size);
}


CFUNC int __libnet_internal__serial_read(struct port *p, unsigned char *buf, int size)
{
    return p->serial->Read(buf, size);
}


static int baudrate(int baud)
{
    /* Aren't baud and bits per second supposed to be different?  Oh well... */
    switch (baud) {
	case 300:    return B_300_BPS;
	case 600:    return B_600_BPS;
	case 1200:   return B_1200_BPS;
	case 2400:   return B_2400_BPS;
	case 9600:   return B_9600_BPS;
	case 19200:  return B_19200_BPS;
	case 38400:  return B_38400_BPS;
	case 57600:  return B_57600_BPS;
	case 115200: return B_115200_BPS;
	default:     return -1;
    }
}

static int bits(int bits)
{
    switch (bits) {
	case 7:  return B_DATA_BITS_7;
	case 8:  return B_DATA_BITS_8;
	default: return -1;
    }
}

static int stopbits(int bits)
{
    switch (bits) {
	case 1:  return B_STOP_BITS_1;
	case 2:  return B_STOP_BITS_2;
	default: return -1;
    }
}

static int parity(char *p) 
{
    if (!strcmp(p, "none")) return B_NO_PARITY;
    if (!strcmp(p, "even")) return B_EVEN_PARITY;
    if (!strcmp(p, "odd"))  return B_ODD_PARITY;
    return -1;
}

CFUNC void __libnet_internal__serial_load_config(int portnum, char *option, char *value)
{
    #define SET(type, value, enumtype)				\
	do {							\
	    int x = type(value);				\
	    if (x != -1) config[portnum].type = (enumtype) x;   \
	} while (0)
    
    if      (!strcmp(option, "baudrate")) SET(baudrate, atoi(value), data_rate);
    else if (!strcmp(option, "bits"))	  SET(bits,	atoi(value), data_bits);
    else if (!strcmp(option, "stopbits")) SET(stopbits, atoi(value), stop_bits);
    else if (!strcmp(option, "parity"))   SET(parity,	value,	     parity_mode);
}


CFUNC int __libnet_internal__serial_init(void) { return 0; }
CFUNC int __libnet_internal__serial_exit(void) { return 0; }


#endif /* __USE_REAL_SERIAL_BEOS__ */
