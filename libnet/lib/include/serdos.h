/*----------------------------------------------------------------
 * serdos.h - header for dos serial routines (internal)
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */


#ifndef libnet_included_file_serdos_h
#define libnet_included_file_serdos_h


/* Bit patterns for control registers.  */

#define BAUD_300        384
#define BAUD_600        192
#define BAUD_1200       96 
#define BAUD_2400       48
#define BAUD_9600       12
#define BAUD_19200      6
#define BAUD_38400      3
#define BAUD_57600      2
#define BAUD_115200     1

#define BITS_5          0
#define BITS_6          1
#define BITS_7          2
#define BITS_8          3

#define PARITY_NONE     0
#define PARITY_ODD      8
#define PARITY_EVEN     24

#define STOPBITS_1      0
#define STOPBITS_2      4


/* UART registers.  */

#define RBR		0	/* read buffer register */
#define THR     	0	/* transmitter holding register */
#define IER     	1	/* interrupt enable register */
#define DLL     	0	/* divisor latch (LSB) */
#define DLH		1	/* divisor latch (MSB) */
#define IIR     	2	/* interrupt identification register */
#define FCR     	2	/* FIFO control register */
#define LCR     	3	/* line control register */
#define MCR     	4	/* modem control register */
#define LSR     	5	/* line status register */
#define MSR     	6	/* modem status register */
#define SCR     	7	/* scratch register */


#define GP02            8       /* enable interrupt */
#define DIV_LATCH_ON    128	/* used to turn reg 0,1 into divisor latch */


/* detect_uart return values.  */

#define UART_8250       1
#define UART_16450      2
#define UART_16550      3
#define UART_16550A     4


#endif
