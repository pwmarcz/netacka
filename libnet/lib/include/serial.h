/*----------------------------------------------------------------
 * serial.h - header for serial routines (internal)
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */


#ifndef libnet_included_file_serial_h
#define libnet_included_file_serial_h


/* Linked list code, inspired by Links, but written from scratch.  */

struct list_head {
    void *next;
    void *prev;
};

#define init_list(L)		do { L.next = L.prev = &L; } while (0)
#define list_empty(L)		(L.next == &L)
#define add_to_list(L, n)	do { n->next = L.next; n->next->prev = n; n->prev = (void *) &L; L.next = n; } while (0)
#define append_to_list(L, n)	do { n->prev = L.prev; n->prev->next = n; n->next = (void *) &L; L.prev = n; } while (0)
#define del_from_list(n)	do { n->prev->next = n->next; n->next->prev = n->prev; } while (0)
#define foreach(n, L)		for (n = L.next; n != (void *) &L; n = n->next)
#define free_list(L, dtor)	do { struct list_head *x, *y; for (x = L.next; x != (void *) &L; x = y) { y = x->next; dtor ((void *) x); } init_list (L); } while (0)


/* Queues.  */

#define QUEUE_SIZE		2048

struct queue {
    unsigned char data[QUEUE_SIZE];
    int head, tail;
};

#define queue_wrap(x)		((x) & (QUEUE_SIZE - 1))
#define queue_full(p)		(queue_wrap (p.head + 1) == p.tail)
#define queue_empty(p)	  	(p.head == p.tail)
#define queue_put(p, c)  	(p.data[p.head] = c, p.head = queue_wrap (p.head + 1))
#define queue_get(p, c)  	(c = p.data[p.tail], p.tail = queue_wrap (p.tail + 1))


/* Low level serial port interface, provided by each platform.  */

int __libnet_internal__serial_init (void);
int __libnet_internal__serial_exit (void);
void __libnet_internal__serial_load_config (int portnum, char *option, char *value);

struct port *__libnet_internal__serial_open (int portnum);
void __libnet_internal__serial_close (struct port *p);

int __libnet_internal__serial_send (struct port *p, const unsigned char *buf, int size);
int __libnet_internal__serial_read (struct port *p, unsigned char *buf, int size);


#endif
