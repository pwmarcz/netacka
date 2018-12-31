/*----------------------------------------------------------------
 * dns.h - definitions for the DNS interface in wnsck.c (internal)
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */


#ifndef _gf_included_dns_h
#define _gf_included_dns_h


/* DNS RR types */
#define DNS_TYPE_A		1
#define DNS_TYPE_NS		2
#define DNS_TYPE_MD		3
#define DNS_TYPE_MF		4
#define DNS_TYPE_CNAME	5
#define DNS_TYPE_SOA	6
#define DNS_TYPE_MB		7
#define DNS_TYPE_MG		8
#define DNS_TYPE_MR		9
#define DNS_TYPE_NULL	10
#define DNS_TYPE_WKS	11
#define DNS_TYPE_PTR	12
#define DNS_TYPE_HINFO	13
#define DNS_TYPE_MINFO	14
#define DNS_TYPE_MX		15
#define DNS_TYPE_TXT	16
#define DNS_TYPE_AXFR	252
#define DNS_TYPE_MAILB	253
#define DNS_TYPE_MAILA	254
#define DNS_TYPE_ANY	255

/* DNS RR classes */
#define DNS_CLASS_IN	1
#define DNS_CLASS_CS	2
#define DNS_CLASS_CH	3
#define DNS_CLASS_HS	4
#define DNS_CLASS_ANY	255

/* DNS communication port (UDP or TCP) */
#define IPPORT_DNS 53

struct dns_query {
	char *qname;
	int qtype,qclass;
};

struct dns_rr {
	char *name;
	int type,_class,ttl,rdlength;
	char *rdata;
	union {
		struct { long address;           } a;
		struct { char *cname;            } cname;
		struct { char *cpu,*os;          } hinfo;
		struct { char *madname;          } mb;
		struct { char *madname;          } md;
		struct { char *madname;          } mf;
		struct { char *mgmname;          } mg;
		struct { char *rmailbx,*emailbx; } minfo;
		struct { char *newname;          } mr;
		struct { int preference; char *exchange; } mx;
		struct { char *nsdname;          } ns;
		struct { char *data;             } null;
		struct { char *ptrdname;         } ptr;
		struct { char *mname,*rname; unsigned serial,refresh,retry,expire,minimum; } soa;
		struct { char **txt_data;        } txt;
		struct { int address; unsigned char protocol; int bitmapsize; char *bitmap; } wks;
	} data;
};

union dns_flags {
	unsigned short i;
	struct {
		int rd:1;
		int tc:1;
		int aa:1;
		int opcode:4;
		int qr:1;
		int rcode:4;
		int z:3;
		int ra:1;
	} f;
};

struct dns_packet {
	int nameserver;
	int id;
	union dns_flags flags;
	int qdcount,ancount,nscount,arcount;
	struct dns_query *questions;
	struct dns_rr *answers;
	struct dns_rr *authorities;
	struct dns_rr *additionals;
};


#endif
