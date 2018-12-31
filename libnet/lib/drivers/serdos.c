/*----------------------------------------------------------------
 * serdos.c - low-level DOS serial routines
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include "platdefs.h"

/* If we can use DOS serial ports, do so. */
#ifdef __USE_REAL_SERIAL_DOS__

#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "serial.h"
#include "serdos.h"


/* We only support this many ports in this driver.  */
#define MAX_PORT	4


/* Port structure.  */
struct port {
    int baseaddr; 
    int irq, vector_num;
    _go32_dpmi_seginfo old_vector;    
    _go32_dpmi_seginfo new_vector;
    int pic_imr;                       /* interrupt mask register */
    int pic_icr;                       /* interrupt control register */
    int interrupt_enable_mask;         /* mask used to enable interrupt */
    int fifo_enabled;    
    struct queue send;
    struct queue recv;
};


/* Default configurations.  */

static struct {
    int baseaddr, irq, baudrate, bits, parity, stopbits;
} config[MAX_PORT] = {
    { -1, 4, BAUD_115200, BITS_8, PARITY_NONE, STOPBITS_1 },
    { -1, 3, BAUD_115200, BITS_8, PARITY_NONE, STOPBITS_1 },
    { -1, 4, BAUD_115200, BITS_8, PARITY_NONE, STOPBITS_1 },
    { -1, 3, BAUD_115200, BITS_8, PARITY_NONE, STOPBITS_1 }
};

static int fallback_baseaddr[MAX_PORT] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8 };


/* Port representations.  */

static struct port port_0, port_1, port_2, port_3;
static struct port *port_table[] = { &port_0, &port_1, &port_2, &port_3 };


/* Wait for a bit.  */

static void _delay(void)
{
    unsigned int end = clock();
    while (end == clock()) ;
}


/* Memory locking.  */

static void _unlock_dpmi_data(void *addr, int size)
{
    unsigned long baseaddr;
    __dpmi_meminfo mem;

    __dpmi_get_segment_base_address(_go32_my_ds(), &baseaddr);
    mem.address = baseaddr + (unsigned long)addr;
    mem.size = size;
    __dpmi_unlock_linear_region(&mem);
}

#define END_OF_STATIC_FUNCTION(x)    	static void x##_end(void) { }
#define LOCK_DATA(d,s)        		_go32_dpmi_lock_data((d), (s))
#define LOCK_CODE(c,s)        		_go32_dpmi_lock_code((c), (s))
#define LOCK_VARIABLE(x)      		LOCK_DATA((void *)&x, sizeof(x))
#define LOCK_FUNCTION(x)      		LOCK_CODE(x, (long)x##_end - (long)x)


/*
 * Enable and disable the THRE interrupt, which tells the ISR to send
 * characters to the serial port.
 */

#define THREINT 0x02

static inline void enable_thre_int(int baseaddr)
{
    unsigned char ch = inportb(baseaddr + IER);
    if (!(ch & THREINT))
	outportb(baseaddr + IER, ch | THREINT);
}

END_OF_STATIC_FUNCTION(enable_thre_int);

static inline void disable_thre_int(int baseaddr)
{
    unsigned char ch = inportb(baseaddr + IER);
    if (ch & THREINT)
	outportb(baseaddr + IER, ch & ~THREINT);
}

END_OF_STATIC_FUNCTION(disable_thre_int);


/*
 * Interrupt service routine (ISR).
 */

static inline void isr(struct port *p)
{
    int cause, len;
    unsigned char ch;

    disable();
    
    /* Loop until all interrupts handled.  */
    while (1) {

	/* Only use lower 3 bits.  */
	cause = inportb(p->baseaddr + IIR) & 0x07;
	if (cause & 0x01)
	    break;

	switch (cause) {
	    /* "OE, PE, FE or BI of the LSR set. 
	     *  Serviced by reading the LSR." */
	    case 0x06:
		inportb(p->baseaddr + LSR);
		break;

	    /* "Receiver DR or trigger level reached. 
	     *  Serviced by reading RBR until under level" */
	    case 0x04:
		ch = inportb(p->baseaddr + RBR);
	    	if (!queue_full(p->recv))
		    queue_put(p->recv, ch);
	    	break;

	    /* "THRE. Serviced by reading IIR (if source of int only!) 
	     *  or writing to THR." */
	    case 0x02:
	    	/* If FIFO is enabled, we can blast up to 16 
		 * bytes into THR at once. */
	    	len = (p->fifo_enabled) ? 16 : 1;
	    
	    	while (!queue_empty(p->send) && (len--)) {
		    queue_get(p->send, ch);
		    outportb(p->baseaddr + THR, ch);
		}

	    	if (queue_empty(p->send))
		    disable_thre_int(p->baseaddr);
		break;

	    /* "One of the delta flags in the MSR set. 
	     *  Serviced by reading MSR." */
	    case 0x00:
		inportb(p->baseaddr + MSR);
		break;

	    default: 
	}
    }

    /* End of interrupt.  */
    outportb(0x20, 0x20);
    if (p->irq > 7)
	outportb(0xa0, 0x20);
}

END_OF_STATIC_FUNCTION(isr);


/*
 * ISR wrappers.
 */

static inline void isr(struct port *p);

#define MAKE_ISR_WRAPPER(X)					\
	static void isr_wrapper_##X(void) { isr(&port_##X); }   \
     	END_OF_STATIC_FUNCTION(isr_wrapper_##X);

MAKE_ISR_WRAPPER(0);
MAKE_ISR_WRAPPER(1);
MAKE_ISR_WRAPPER(2);
MAKE_ISR_WRAPPER(3);

static void (*isr_wrapper_table[])(void) = {
    &isr_wrapper_0, &isr_wrapper_1,
    &isr_wrapper_2, &isr_wrapper_3
};


/*
 * Write to port.
 */

int __libnet_internal__serial_send(struct port *port,
				   const unsigned char *buf, int size)
{
    const unsigned char *p = buf;
    while (!queue_full(port->send) && (size--))
	queue_put(port->send, *p++);

    enable_thre_int(port->baseaddr);

    return p - buf;
}

static void flush_send_buffer(struct port *p)
{
    while (!queue_empty(p->send)) {
	enable_thre_int(p->baseaddr);
	_delay();
    }

    disable_thre_int(p->baseaddr);
}


/*
 * Read from port.
 */

int __libnet_internal__serial_read(struct port *port, unsigned char *buf, 
				   int size)
{    
    unsigned char *p = buf;
    while (!queue_empty(port->recv) && (size--))
	queue_get(port->recv, *p++);
    return p - buf;
}


/*
 * Init / shutdown driver.
 */

int __libnet_internal__serial_init(void)
{
    LOCK_FUNCTION(enable_thre_int);
    LOCK_FUNCTION(disable_thre_int);
    LOCK_FUNCTION(isr);
    LOCK_FUNCTION(isr_wrapper_0);
    LOCK_FUNCTION(isr_wrapper_1);
    LOCK_FUNCTION(isr_wrapper_2);
    LOCK_FUNCTION(isr_wrapper_3);
    LOCK_VARIABLE(port_0);
    LOCK_VARIABLE(port_1);
    LOCK_VARIABLE(port_2);
    LOCK_VARIABLE(port_3);
    return 0;
}

int __libnet_internal__serial_exit(void) 
{
    return 0; 
}


/* 
 * Detect what type of UART exists at a particular address.
 * ONLY call this before the ISR is installed for that port.
 */

static int detect_uart(unsigned int baseaddr)
{
    int x, olddata, temp;

    /* check if a UART is present */
    olddata = inportb(baseaddr + MCR);

    /* enable loopback mode, set RTS & DTR to 1 */
    outportb(baseaddr + MCR, 0x1f);
    _delay();

    /* Read the state of RTS and DTR. 
     * Do this twice, so that lower 4 bits are clear. 
     * OS/2 returns 0xB0 after this, instead of 0xff if no port is there.
     */
    disable();
    temp = inportb(baseaddr + MSR);
    temp = inportb(baseaddr + MSR);
    enable();

    if ((temp & 0x3f) != 0x30)
	return 0;

    /* restore RTS & DTR */
    outportb(baseaddr + MCR, olddata);
    _delay();

    /* next thing to do is look for the scratch register */
    olddata = inportb(baseaddr + SCR);

    outportb(baseaddr + SCR, 0x55);
    _delay();
    if (inportb(baseaddr + SCR) != 0x55)
	return UART_8250;

    outportb(baseaddr + SCR, 0xAA);
    _delay();
    if (inportb(baseaddr + SCR) != 0xAA)
	return UART_8250;

    /* restore it if it's there */
    outportb(baseaddr + SCR, olddata);
    _delay();

    /* check if there's a FIFO */
    outportb(baseaddr + IIR, 1);
    _delay();
    x = inportb(baseaddr + IIR);

    /* some old-fashioned software relies on this! */
    outportb(baseaddr + IIR, 0x0);
    _delay();

    if ((x & 0x80) == 0) return UART_16450;
    if ((x & 0x40) == 0) return UART_16550;
    return UART_16550A;
}


/*
 * Enable / disable FIFOs.
 */

static void enable_fifo(struct port *p)
{
    /* 0xC7: 14 byte trigger level */
    /* 0x87: 08 byte trigger level */
    outportb(p->baseaddr + FCR, 0x87);
    p->fifo_enabled = 1;
}

static void disable_fifo(struct port *p)
{
    outportb(p->baseaddr + FCR, 0);
    p->fifo_enabled = 0;
}


/*
 * Install / uninstall ISRs.
 */

static void install_isr(int portnum, struct port *p, int irq)
{
    int x, baseaddr = p->baseaddr;
    
    p->irq = irq;

    /* Clear pending interrupts.  */
    do {
	inportb(baseaddr + RBR);
	inportb(baseaddr + LSR);
	inportb(baseaddr + MSR);
	x = inportb(baseaddr + IIR);
    } while (!(x & 0x01));  

    /* Calculate vector from IRQ.  */
    if (p->irq <= 7) {
	p->interrupt_enable_mask = ~(1 << p->irq);	/* IMR is inverted */
	p->vector_num = 8 + p->irq;
	p->pic_imr = 0x21;
    }
    else {
	p->interrupt_enable_mask = ~(1 << (p->irq - 8));
	p->vector_num = 0x70 + (p->irq - 8);
	p->pic_imr = 0xa1;
    }

    /* Install ISR into vector.  */
    p->new_vector.pm_selector = _go32_my_cs();
    p->new_vector.pm_offset = (int)isr_wrapper_table[portnum];
    _go32_dpmi_allocate_iret_wrapper(&p->new_vector);

    disable();

    _go32_dpmi_get_protected_mode_interrupt_vector(p->vector_num, &p->old_vector);
    _go32_dpmi_set_protected_mode_interrupt_vector(p->vector_num, &p->new_vector);

    /* Enable interrupts.  */
    outportb(baseaddr + MCR, 0x0f);
    outportb(baseaddr + IER, 0x0f);
    outportb(p->pic_imr, inportb(p->pic_imr) & p->interrupt_enable_mask);
    
    enable();
}

static void uninstall_isr(struct port *p)
{
    /* Disable interrupts.  */
    outportb(p->baseaddr + MCR, 0);
    outportb(p->baseaddr + IER, 0);
    outportb(p->pic_imr, inportb(p->pic_imr) | (~p->interrupt_enable_mask));

    /* Restore old vector.  */
    disable();
    _go32_dpmi_set_protected_mode_interrupt_vector(p->vector_num, &p->old_vector);
    _go32_dpmi_free_iret_wrapper(&p->new_vector);
    enable();
}

static void set_config(int baseaddr, int baudrate, int config)
{
    /* Turn on divisor latch registers.  */
    outportb(baseaddr + LCR, DIV_LATCH_ON);

    /* Send low and high bytes to divisor latches.  */
    outportb(baseaddr + DLL, baudrate);
    outportb(baseaddr + DLH, 0);

    /* Set the configuration for the port.  */
    outportb(baseaddr + LCR, config & 0x7F);
}


/*
 * Open and close port.
 */

struct port *__libnet_internal__serial_open(int portnum)
{
    struct port *p;
    int uart, baudrate, cfg;
    
    if ((portnum < 0) || (portnum >= MAX_PORT))
	return NULL;

    p = port_table[portnum];
    memset(p, 0, sizeof *p);

    if (config[portnum].baseaddr == -1) {
	dosmemget(0x400 + portnum * 2, 2, &p->baseaddr);
	if (p->baseaddr == 0)
	    p->baseaddr = fallback_baseaddr[portnum];
    }
    else {
	p->baseaddr = config[portnum].baseaddr;
    }

    uart = detect_uart(p->baseaddr);
    if (!uart)
	return NULL;

    if (uart == UART_16550A)
	enable_fifo(p);
    else 
	disable_fifo(p);

    install_isr(portnum, p, config[portnum].irq);

    set_config(p->baseaddr, config[portnum].baudrate, 
	       (config[portnum].bits 
		| config[portnum].parity 
		| config[portnum].stopbits));

    return p;
}

void __libnet_internal__serial_close(struct port *p)
{
    flush_send_buffer(p);
    if (p->fifo_enabled) 
	disable_fifo(p);
    uninstall_isr(p);
}


/*
 * Config stuff.
 */

static int my_atoi(const char *s)
{
    return (isdigit(*s)) ? atoi(s) : -1;
}

static int baudrate(int baud)
{
    switch (baud) {
	case 300:    return BAUD_300;
	case 600:    return BAUD_600;
	case 1200:   return BAUD_1200;
	case 2400:   return BAUD_2400;
	case 9600:   return BAUD_9600;
	case 19200:  return BAUD_19200;
	case 38400:  return BAUD_38400;
	case 57600:  return BAUD_57600;
	case 115200: return BAUD_115200;
	default:     return -1;
    }
}

static int bits(int b)
{
    switch (b) {
	case 5:  return BITS_5;
	case 6:  return BITS_6;
	case 7:  return BITS_7;
	case 8:  return BITS_8;
	default: return -1;
    }
}

static int stopbits(int sb)
{
    switch (sb) {
	case 1:  return STOPBITS_1;
	case 2:  return STOPBITS_2;
	default: return -1;
    }
}

static int parity(char *p)
{
    if	    (!strcmp(p, "none")) return PARITY_NONE;
    else if (!strcmp(p, "odd"))  return PARITY_ODD;
    else if (!strcmp(p, "even")) return PARITY_EVEN;
    else return -1;
}

static int baseaddr(int x) { return x; }
static int irq(int x) { return x; }


void __libnet_internal__serial_load_config(int portnum, char *option, char *value)
{
    int x;
    
    #define SET(type, value)				\
	do {						\
	    x = type(value);				\
	    if (x != -1) config[portnum].type = x;      \
	} while (0)
 
    if      (strcmp(option, "baudrate") == 0)  SET(baudrate,	atoi(value));
    else if (strcmp(option, "bits")	== 0)  SET(bits,	atoi(value));
    else if (strcmp(option, "stopbits") == 0)  SET(stopbits,	atoi(value));
    else if (strcmp(option, "parity")	== 0)  SET(parity,	value);
    else if (strcmp(option, "baseaddr") == 0)  SET(baseaddr,	my_atoi(value));
    else if (strcmp(option, "irq")	== 0)  SET(irq,		my_atoi(value));
}


#endif /* __USE_REAL_SERIAL_DOS__ */


/*
 *  Originally based on SK 0.7d, which was based on Andre' LaMothe's
 *  routines from the book `Tricks of the Game Programming Gurus'.
 *
 *  A lot of code and ideas from DZComm, by Dim Zegebart.
 *
 *  Technical info and sample code from `The Serial Port' by Chris Blum.
 *
 *  Memory locking stuff from Allegro by Shawn Hargreaves.
 * 
 *  Modifications to detect_uart by Ralph Deane.
 *
 *  Bug causing a crash when receiving too many data fixed by Ben Davis.
 */
