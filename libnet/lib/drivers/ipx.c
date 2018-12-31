/*----------------------------------------------------------------
 * ipx.c - IPX network driver for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include "platdefs.h"

/* If this platform supports Dos style IPX use them. Otherwise use
 * a dummy driver
 */

#ifdef __USE_REAL_IPX_DOS__

/* ugh, I really hate the nearptr hack... */
#include <sys/nearptr.h>
#define DPMI_real_segment(P) ((((uint)(P)-(uint)(__djgpp_conventional_base)) >> 4) & 0xffff)
#define DPMI_real_offset(P) (((uint)(P)-(uint)(__djgpp_conventional_base)) & 0xf)
#include <crt0.h>

#include <mem.h>
#include <string.h>
#include <go32.h>
#include <dpmi.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "libnet.h"
#include "internal.h"
#include "config.h"
#include "types.h"

static int ipx_init( int socket_number, int show_address, int detectonly );
static void ipx_close(void);
static int ipx_change_default_socket( ushort socket_number );
static ubyte *ipx_get_my_local_address(void);
static ubyte *ipx_get_my_server_address(void);
static void ipx_get_local_target( ubyte * server, ubyte * node, ubyte * local_target );
static int ipx_get_packet_data( ubyte * data, int detectonly, int maxsize, int channel );
static void ipx_send_packet_data( ubyte * data, int datasize, ubyte *network, ubyte *address, ubyte *immediate_address );
static void ipx_send_internetwork_packet_data( ubyte * data, int datasize, ubyte * server, ubyte *address );

typedef struct PORTstruct PORT, *PORTp;
typedef struct PORTstruct               /* link structure */
{
  PORTp next;                          /* linked list next node */
  PORTp prev;                          /* linked list previous node */
  int port;
} PORTstruct;

static int add_port(int port);
static void kill_port(int port);
static PORTp findport(int port);

static PORTp top_nodel, bottom_nodel;   /* nodes in linked list */
static int next_port=1;

static DWORD ipx_network = 0;
static ubyte ipx_myy_node[6];
static ubyte ipx_from_node[6], ipx_from_server[4];

static char dosipx_desc[80] = "IPX network driver";

static int add_port(int port)
{
  PORTp objp;

  objp=findport(port);
  if(objp!=NULL)
    return 0;
  objp=malloc(sizeof(PORT));
  objp->port=port;
  if (bottom_nodel==(PORTp)NULL)
  {
    bottom_nodel=objp;
    objp->prev=(PORTp)NULL;
  }
  else
  {
    objp->prev=top_nodel;
    objp->prev->next=objp;
  }
  top_nodel=objp;
  objp->next=(PORTp)NULL;
  return 1;
}

static void kill_port(int port)
{
  PORTp node, objp;

  objp=findport(port);
  if(objp==NULL)
    return;
  node=objp;
  if (node==bottom_nodel)
  {
    bottom_nodel=node->next;
    if (bottom_nodel!=(PORTp)NULL) bottom_nodel->prev=(PORTp)NULL;
  }
  else if (node==top_nodel)
  {
    top_nodel=node->prev;
    top_nodel->next=(PORTp)NULL;
  }
  else
  {
    node->prev->next=node->next;
    node->next->prev=node->prev;
  }
  free(node);
}

static PORTp findport(int port)
{
  PORTp nodel;
  PORTp next_nodel;

  if(bottom_nodel!=NULL)
  {
    for (nodel=bottom_nodel;nodel!=(PORTp)NULL;nodel=next_nodel)
    {
      next_nodel=nodel->next;
      if(port==nodel->port)
        return nodel;
    } 
  }
  return NULL;
}

/* disable_driver: set by the config loader to disable this driver */
static int disable_driver = 0;

/* detect:
 *  This function returns one of the following:
 *
 *  - NET_DETECT_YES    (if the network driver can definitely work)
 *  - NET_DETECT_NO     (if the network driver definitely cannot work)
 */

static int dosipx_detect (void)
{ 
  if (disable_driver) return NET_DETECT_NO;
  if(ipx_init(0,0,1)!=0)
    return NET_DETECT_NO;
  return NET_DETECT_YES;
}

/* init:
 *  Initialises the driver.  Return zero if successful, non-zero if not.
 */
static int dosipx_init (void) 
{
  top_nodel=NULL;
  bottom_nodel=NULL;
  return ipx_init(0x869d,0,0); 
}

static int dosipx_exit (void) 
{
  PORTp nodel;
  PORTp next_nodel;

  ipx_close();
  if(bottom_nodel!=NULL)
  {
    for (nodel=bottom_nodel;nodel!=(PORTp)NULL;nodel=next_nodel)
    {
      next_nodel=nodel->next;
      kill_port(nodel->port);
    } 
  }
  return 0; 
}

static int dosipx_init_channel    (NET_CHANNEL *chan, const char *addr) 
{
  ubyte *ipx_real_buffer, *ipx_real_bufferx;
  ubyte broadcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  int port;
  char buffer[80];

  if (!(chan->data = malloc(20))) return -1;
  /* binding specifies a channel # for the channel
   * "" is default port 0
   * NULL is next avalible port
   * :port is port
   */
  if(addr==NULL)
  {
    do
    {
      port=next_port;
      next_port+=1;
    } while (findport(port)!=NULL);
    next_port=1;
  }
  else
  {
    if (strcmp(addr,"")==0)
    {
      port=0;
    }
    else
    {
      if(addr[0]==':')
      {
        port=atoi(addr+1);
      }
      else
      {
        return 1;
      }
    }
  }
  if(add_port(port)==0)
    return 1;
  ipx_real_buffer=(ubyte *)&ipx_network;
  ipx_real_bufferx=(ubyte *)&ipx_myy_node;
  sprintf(chan->local_addr,"%02X%02X%02X%02X/%02X%02X%02X%02X%02X%02X:",ipx_real_buffer[0],ipx_real_buffer[1],ipx_real_buffer[2],ipx_real_buffer[3],ipx_real_bufferx[0],ipx_real_bufferx[1],ipx_real_bufferx[2],ipx_real_bufferx[3],ipx_real_bufferx[4],ipx_real_bufferx[5] );
  itoa(port,buffer,10);
  strcat(chan->local_addr,buffer);
  memcpy(chan->data,ipx_real_buffer,4); // send to local network
  memcpy(chan->data+4,broadcast,6);     // broadcast to all
  memcpy(chan->data+10,&port,4); // your channel
  memset(chan->data+14,0,4); // channel to send to
  return 0; 
}

static int dosipx_destroy_channel (NET_CHANNEL *chan) 
{ 
  int port;

  memcpy(&port,chan->data+10,4);
  kill_port(port);
  free(chan->data);
  return 0;
}


static int dosipx_update_target   (NET_CHANNEL *chan) 
{
  char *data, rad[3], **stop_at=NULL;
  ubyte tmpadr[14];
  int i, offset;
  ubyte *ipx_real_buffer;

  data=chan->target_addr;
  if(strlen(data)<12)
    return 1;
  offset=0;
  ipx_real_buffer=(ubyte *)&ipx_network;
  memcpy(chan->data,ipx_real_buffer,4);
  if(data[8]=='/')
  {
    rad[2]=0;
    for(i=0;i<4;i++)
    {
      strncpy(rad,(data+(i*2)),2);
      tmpadr[i] = (ubyte)strtol(rad,stop_at,16);
    }
    memcpy(chan->data,tmpadr,4);
    offset=9;
  }
  rad[2]=0;
  for(i=0;i<6;i++)
  {
    strncpy(rad,(data+(i*2)+offset),2);
    tmpadr[i] = (ubyte)strtol(rad,stop_at,16);
  }
  memcpy(chan->data+4,tmpadr,6);
  memset(chan->data+14,0,4);
  if(data[offset+12]==':')
  {
    i=atoi(data+offset+13);
    memcpy((chan->data+14),&i,4);
  }
  return 0; 
}

static int dosipx_send (NET_CHANNEL *chan, const void *buf, int size) 
{
  ubyte *packet;

  packet=malloc(size+8);
  memcpy(packet,(chan->data+10),8);
  memcpy((packet+8),buf,size);
  ipx_send_internetwork_packet_data(packet, size+8, chan->data, (chan->data+4));
  free(packet);
  return 0; 
}

static int dosipx_recv (NET_CHANNEL *chan, void *buf, int size, char *from) 
{
  int datasize;
  ubyte *ipx_real_buffer, *ipx_real_bufferx;
  ubyte *packet;
  int channel;
  char buffer[80];

  packet=malloc(size+8);
  memcpy(&channel,(chan->data+10),4);
  datasize=ipx_get_packet_data(packet, 0, size+8, channel);
  if(datasize==0)
  {
    free(packet);
    return 0;
  }
  memcpy(buf,(packet+8),datasize-8);
  if(from!=NULL)
  {
    ipx_real_buffer=(ubyte *)&ipx_from_server;
    ipx_real_bufferx=(ubyte *)&ipx_from_node;
    sprintf(from,"%02X%02X%02X%02X/%02X%02X%02X%02X%02X%02X:",ipx_real_buffer[0],ipx_real_buffer[1],ipx_real_buffer[2],ipx_real_buffer[3],ipx_real_bufferx[0],ipx_real_bufferx[1],ipx_real_bufferx[2],ipx_real_bufferx[3],ipx_real_bufferx[4],ipx_real_bufferx[5] );
    memcpy(&channel,packet,4);
    itoa(channel,buffer,10);
    strcat(from,buffer);
  }
  free(packet);
  return (datasize-8); 
}

/* query:
 *  Returns non-zero if there is data waiting to be read from the channel.
 */
static int dosipx_query (NET_CHANNEL *chan)
{
  int channel;

  memcpy(&channel,(chan->data+10),4);
  return (ipx_get_packet_data(NULL,1,0,channel));
}


/* load_config:
 *  This will be called once when the library is initialised, inviting the
 *  driver to load configuration information from the passed text file.
 */
static void dosipx_load_config (NET_DRIVER *drv, FILE *fp) 
{ 
  char *option, *value, **stop_at=NULL;
  int x;

  if (__libnet_internal__seek_section (fp, "ipx")) return;
  
  while (__libnet_internal__get_setting (fp, &option, &value) == 0) {
    
    if (!strcmp (option, "socket")) {
      x = strtol (value,stop_at,0);
      ipx_change_default_socket((ushort)x);
    } else if (!strcmp (option, "disable")) {
      disable_driver = (atoi (value) || (value[0] == 'y'));
    }
  }
  
  if (drv->load_conn_config) drv->load_conn_config (drv, fp, "ipx");
}


NET_DRIVER net_driver_ipx_dos = {
	"IPX", 
	dosipx_desc,
	NET_CLASS_IPX,

	dosipx_detect,
	dosipx_init,
	dosipx_exit,

	NULL, NULL,

	dosipx_init_channel,
	dosipx_destroy_channel,

	dosipx_update_target,
	dosipx_send,
	dosipx_recv,
	dosipx_query,

	NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL,

	dosipx_load_config,
	NULL, NULL
};

#define IPX_MAX_DATA_SIZE (1024)		

static inline unsigned int swap_short( unsigned int sshort ) {
	asm("xchgb %h0,%b0"
         : "=q" (sshort) 
         : "0" (sshort));
	return sshort;
}

typedef struct local_address {
	ubyte address[6];
} __pack__ local_address;

typedef struct net_address {
	BYTE		network_id[4];			
	local_address	node_id;
	WORD		socket_id;
} __pack__ net_address;

typedef struct ipx_header {
	WORD		checksum;
	WORD		length;
	BYTE		transport_control;
	BYTE		packet_type;
	net_address	destination;
	net_address	source;
} __pack__ ipx_header;

typedef struct ecb_header {
	WORD		link[2];
	WORD		esr_address[2];
	volatile BYTE	in_use;
	BYTE		completion_code;
	WORD		socket_id;
	BYTE		ipx_reserved[14];        
	WORD		connection_id;
	local_address   immediate_address;
	WORD    	fragment_count;
	WORD		fragment_pointer[2];
	WORD		fragment_size;
} __pack__ ecb_header;

typedef struct packet_data {
	int		packetnum;
        ubyte           ipx_from_node[6];
        ubyte           ipx_from_server[4];
	byte		data[IPX_MAX_DATA_SIZE];
} __pack__ packet_data;

typedef struct ipx_packet {
	ecb_header	ecb;
	ipx_header	ipx;
	packet_data	pd;
} __pack__ ipx_packet;

typedef struct user_address {
	ubyte network[4];
	ubyte node[6];
	ubyte address[6];
} __pack__ user_address;

typedef struct {
	ubyte 	network[4];
	ubyte	node[6];
	ubyte	local_target[6];
} __pack__ net_xlat_info;

#define MAX_NETWORKS 64
static int Ipx_num_networks = 0;
static uint Ipx_networks[MAX_NETWORKS];

static int ipx_packetnum = 0;

#define MAX_PACKETS 64

static packet_data packet_buffers[MAX_PACKETS];
static short packet_free_list[MAX_PACKETS];
static int num_packets = 0;
static int largest_packet_index = 0;
static short packet_size[MAX_PACKETS];

static WORD ipx_socket=0;
static ubyte ipx_installed=0;
static WORD ipx_vector_segment;
static WORD ipx_vector_offset;
static ubyte ipx_socket_life = 0; 	// 0=closed at prog termination, 0xff=closed when requested.
static local_address ipx_my_node;
static WORD ipx_num_packets=32;		// 32 Ipx packets
static ipx_packet * packets;
static int neterrors = 0;
static int ipx_packets_selector;

static ecb_header * last_ecb=NULL;
static int lastlen=0;

static void got_new_packet( ecb_header * ecb );
static void ipx_listen_for_packet(ecb_header * ecb );
static void free_packet( int id );
static void ipx_cancel_listen_for_packet(ecb_header * ecb );
static void ipx_send_packet(ecb_header * ecb );

static void free_packet( int id )
{
  packet_buffers[id].packetnum = -1;
  packet_free_list[ --num_packets ] = id;
  if (largest_packet_index==id)	
    while ((--largest_packet_index>0) && (packet_buffers[largest_packet_index].packetnum == -1 ));
}

static int ipx_get_packet_data( ubyte * data, int detectonly, int maxsize, int channel )
{
  int i, best, best_id, size;
  int n0, n1;

  for (i=1; i<ipx_num_packets; i++ )
  {
    if ( !packets[i].ecb.in_use )
    {
      got_new_packet( &packets[i].ecb );
      packets[i].ecb.in_use = 0;
      ipx_listen_for_packet(&packets[i].ecb);
    }			
  }

  best = -1;
  best_id = -1;
  for (i=0; i<=largest_packet_index; i++ )
  {
    if ( packet_buffers[i].packetnum > -1 )
    {
      if ( best == -1 || (packet_buffers[i].packetnum<best) )
      {
        memcpy(&n0,packet_buffers[i].data,4);
        memcpy(&n1,(packet_buffers[i].data+4),4);
        if(n0==channel && n1==channel && memcmp( &packet_buffers[i].ipx_from_node, &ipx_my_node, 6 )==0)
        {
          free_packet(i);
        }
        else
        {
          if((n0!=channel || memcmp( &packet_buffers[i].ipx_from_node, &ipx_my_node, 6 )) && n1==channel)
          {
            best = packet_buffers[i].packetnum;
            best_id = i;
          }
        }
      }
    }			
  }
  if ( best_id < 0 ) return 0;
  if(detectonly==1) return 1;
  size = packet_size[best_id];
  if(size>maxsize)
    size=maxsize;
  memcpy( data, packet_buffers[best_id].data, size );
  memcpy( &ipx_from_node,&packet_buffers[best_id].ipx_from_node, 6 );
  memcpy( &ipx_from_server,&packet_buffers[best_id].ipx_from_server, 4 );
  free_packet(best_id);
  return size;
}

static void got_new_packet( ecb_header * ecb )
{
  ipx_packet * p;
  int id;
  unsigned short datasize;

  datasize = 0;
  last_ecb = ecb;
  p = (ipx_packet *)ecb;

  if ( p->ecb.in_use ) 
  { 
    neterrors++;
    return; 
  }
  if( p->ecb.completion_code )
  { 
    neterrors++;
    return;
  }


  datasize=swap_short(p->ipx.length);
  lastlen=datasize;
  datasize -= sizeof(ipx_header);
  // Find slot to put packet in...
  if ( datasize > 0 && datasize <= sizeof(packet_data) )
  {
    if ( num_packets >= MAX_PACKETS ) 
    {
      neterrors++;
      return;
    }		
    id = packet_free_list[ num_packets++ ];
    if (id > largest_packet_index ) largest_packet_index = id;
    packet_size[id] = datasize-sizeof(int)-(10*sizeof(ubyte));
    packet_buffers[id].packetnum =  p->pd.packetnum;
    if ( packet_buffers[id].packetnum < 0 )
    {
      neterrors++;
      return; 
    }
    memcpy( &packet_buffers[id].ipx_from_node, &p->ipx.source.node_id, 6 );
    memcpy( &packet_buffers[id].ipx_from_server, &p->ipx.source.network_id, 4 );
    memcpy( packet_buffers[id].data, p->pd.data, packet_size[id] );
  } 
  else
  {
    neterrors++;
    return;
  }
  // Repost the ecb
  p->ecb.in_use = 0;
}

static ubyte *ipx_get_my_local_address(void)
{
  return ipx_my_node.address;
}

static ubyte *ipx_get_my_server_address(void)
{
  return (ubyte *)&ipx_network;
}

static void ipx_listen_for_packet(ecb_header * ecb )	
{
	__dpmi_regs regs;
	ecb->in_use = 0x1d;
	memset(&regs,0,sizeof regs);
	regs.d.ebx = 4;	// Listen For Packet function
	regs.d.esi = DPMI_real_offset(ecb);
	regs.x.es = DPMI_real_segment(ecb);
	__dpmi_int( 0x7a, &regs );
}

static void ipx_cancel_listen_for_packet(ecb_header * ecb )	
{
	__dpmi_regs regs;
	memset(&regs,0,sizeof regs);
	regs.d.ebx = 6;	// IPX Cancel event
	regs.d.esi = DPMI_real_offset(ecb);
	regs.x.es = DPMI_real_segment(ecb);
	__dpmi_int( 0x7A, &regs );
}


static void ipx_send_packet(ecb_header * ecb )	
{
	__dpmi_regs regs;
	memset(&regs,0,sizeof regs);
	regs.d.ebx = 3;	// Send Packet function
	regs.d.esi = DPMI_real_offset(ecb);
	regs.x.es = DPMI_real_segment(ecb);
	__dpmi_int( 0x7A, &regs );
}

static void ipx_get_local_target( ubyte * server, ubyte * node, ubyte * local_target )
{
	net_xlat_info * info;
	__dpmi_regs regs;
		
	// Get dos memory for call...
	info = (net_xlat_info *) (((__tb + 15) & ~15) + __djgpp_conventional_base);
	assert( info != NULL );
	memcpy( info->network, server, 4 );
	memcpy( info->node, node, 6 );
	
	memset(&regs,0,sizeof regs);

	regs.d.ebx = 2;		// Get Local Target	
	regs.x.es = DPMI_real_segment(info);
	regs.d.esi = DPMI_real_offset(info->network);
	regs.d.edi = DPMI_real_offset(info->local_target);

	__dpmi_int( 0x7A, &regs );

	// Save the local target...
	memcpy( local_target, info->local_target, 6 );
}

static void ipx_close(void)
{
	__dpmi_regs regs;
	if ( ipx_installed )	{
		// When using VLM's instead of NETX, the sockets don't
		// seem to automatically get closed, so we must explicitly
		// close them at program termination.
		ipx_installed = 0;
		memset(&regs,0,sizeof regs);
		regs.d.edx = ipx_socket;
		regs.d.ebx = 1;	// Close socket
		__dpmi_int( 0x7A, &regs );
	}
}

//---------------------------------------------------------------
// Initializes all IPX internals. 
// If socket_number==0, then opens next available socket.
// Returns:	0  if successful.
//		-1 if socket already open.
//		-2 if socket table full.
//		-3 if IPX not installed.
//		-4 if couldn't allocate low dos memory
//		-5 if error with getting internetwork address
//              -6 if nearptrs not available

static int ipx_init( int socket_number, int show_address, int detectonly )
{
	__dpmi_regs regs;
	ubyte *ipx_real_buffer;
	int i;

	if (~_crt0_startup_flags & _CRT0_FLAG_NONMOVE_SBRK) return -6;
	if (~_crt0_startup_flags & _CRT0_FLAG_NEARPTR)
		if (!__djgpp_nearptr_enable()) return -6;

	atexit(ipx_close);

	ipx_packetnum = 0;

	// init packet buffers.
	for (i=0; i<MAX_PACKETS; i++ )	{
		packet_buffers[i].packetnum = -1;
		packet_free_list[i] = i;
	}
	num_packets = 0;
	largest_packet_index = 0;

	// Get the IPX vector
	memset(&regs,0,sizeof regs);
	regs.d.eax=0x00007a00;
	__dpmi_int( 0x2f, &regs );

	if ( (regs.d.eax & 0xFF) != 0xFF )	{
		return 3;   
	}
        if(detectonly==1) return 0;

	ipx_vector_offset = regs.d.edi & 0xFFFF;
	ipx_vector_segment = regs.x.es;

	// Open a socket for IPX

	memset(&regs,0,sizeof regs);
	swab( (char *)&socket_number,(char *)&ipx_socket, 2 );
	regs.d.edx = ipx_socket;
	regs.d.eax = ipx_socket_life;
	regs.d.ebx = 0;	// Open socket
	__dpmi_int( 0x7A, &regs );
	
	ipx_socket = regs.d.edx & 0xFFFF;
	
	if ( regs.d.eax & 0xFF )	{
		return -2;
	}
	
	ipx_installed = 1;

	// Find our internetwork address
	ipx_real_buffer =  (void *) (((__tb + 15) & ~15) + __djgpp_conventional_base); /* need 1k */
	if ( ipx_real_buffer == NULL )	{
		return -4;
	}

	memset(&regs,0,sizeof regs);
	regs.d.ebx = 9;		// Get internetwork address
	regs.d.esi = DPMI_real_offset(ipx_real_buffer);
	regs.x.es = DPMI_real_segment(ipx_real_buffer);
	__dpmi_int( 0x7A, &regs );

	if ( regs.d.eax & 0xFF )	{
		return -2;
	}

	memcpy( &ipx_network, ipx_real_buffer, 4 );
	memcpy( &ipx_my_node, &ipx_real_buffer[4], 6 );
	memcpy( &ipx_myy_node, &ipx_real_buffer[4], 6 );

	Ipx_num_networks = 0;
	memcpy( &Ipx_networks[Ipx_num_networks++], &ipx_network, 4 );

	if ( show_address )	{
		printf( "My IPX addresss is " );
		printf( "%02X%02X%02X%02X/", ipx_real_buffer[0],ipx_real_buffer[1],ipx_real_buffer[2],ipx_real_buffer[3] );
		printf( "%02X%02X%02X%02X%02X%02X\n", ipx_real_buffer[4],ipx_real_buffer[5],ipx_real_buffer[6],ipx_real_buffer[7],ipx_real_buffer[8],ipx_real_buffer[9] );
		printf( "\n" );
	}

	{
		int result = __dpmi_allocate_dos_memory ((sizeof(ipx_packet)*ipx_num_packets + 15)/16, &ipx_packets_selector);
		if (result == -1)
			return -4;
		packets = (void *) ((result << 4) + __djgpp_conventional_base);
	}
	memset( packets, 0, sizeof(ipx_packet)*ipx_num_packets );

	for (i=1; i<ipx_num_packets; i++ )	{
		packets[i].ecb.in_use = 0x1d;
		packets[i].ecb.socket_id = ipx_socket;
		packets[i].ecb.fragment_count = 1;
		packets[i].ecb.fragment_pointer[0] = DPMI_real_offset(&packets[i].ipx);
		packets[i].ecb.fragment_pointer[1] = DPMI_real_segment(&packets[i].ipx);
		packets[i].ecb.fragment_size = sizeof(ipx_packet)-sizeof(ecb_header);			//-sizeof(ecb_header);

		ipx_listen_for_packet(&packets[i].ecb);
	}

	packets[0].ecb.socket_id = ipx_socket;
	packets[0].ecb.fragment_count = 1;
	packets[0].ecb.fragment_pointer[0] = DPMI_real_offset(&packets[0].ipx);
	packets[0].ecb.fragment_pointer[1] = DPMI_real_segment(&packets[0].ipx);
	packets[0].ipx.packet_type = 4;		// IPX packet
	packets[0].ipx.destination.socket_id = ipx_socket;
	memset( packets[0].ipx.destination.network_id, 0, 4 );

	return 0;
}

static void ipx_send_packet_data( ubyte * data, int datasize, ubyte *network, ubyte *address, ubyte *immediate_address )
{
  assert(ipx_installed);
  if ( datasize >= IPX_MAX_DATA_SIZE )
    datasize = IPX_MAX_DATA_SIZE;
  if(datasize<1)
    return;

  // Make sure no one is already sending something
  while( packets[0].ecb.in_use )
  {
  }
  if (packets[0].ecb.completion_code)
  {
    printf( "Send error %d for completion code\n", packets[0].ecb.completion_code );
    exit(1);
  }

	// Fill in destination address
	if ( memcmp( network, &ipx_network, 4 ) )
		memcpy( packets[0].ipx.destination.network_id, network, 4 );
	else
		memset( packets[0].ipx.destination.network_id, 0, 4 );
	memcpy( packets[0].ipx.destination.node_id.address, address, 6 );
	memcpy( packets[0].ecb.immediate_address.address, immediate_address, 6 );
	packets[0].pd.packetnum = ipx_packetnum++;

	// Fill in data to send
	packets[0].ecb.fragment_size = sizeof(ipx_header) + sizeof(int) + 10*sizeof(ubyte) + datasize;

	memcpy( packets[0].pd.data, data, datasize );

	// Send it
	ipx_send_packet( &packets[0].ecb );

}

// Sends a non-localized packet... needs 4 byte server, 6 byte address
static void ipx_send_internetwork_packet_data( ubyte * data, int datasize, ubyte * server, ubyte *address )
{
	ubyte local_address[6];

	if ( (*(uint *)server) != 0 )	{
		ipx_get_local_target( server, address, local_address );
		ipx_send_packet_data( data, datasize, server, address, local_address );
	} else {
		// Old method, no server info.
		ipx_send_packet_data( data, datasize, server, address, address );
	}
}

static int ipx_change_default_socket( ushort socket_number )
{
	int i;
	WORD new_ipx_socket;
	__dpmi_regs regs;

	if ( !ipx_installed ) return -3;

	// Open a new socket	
	memset(&regs,0,sizeof regs);
	swab( (char *)&socket_number,(char *)&new_ipx_socket, 2 );
	regs.d.edx = new_ipx_socket;
	regs.d.eax = ipx_socket_life;
	regs.d.ebx = 0;	// Open socket
	__dpmi_int( 0x7A, &regs );
	
	new_ipx_socket = regs.d.edx & 0xFFFF;
	
	if ( regs.d.eax & 0xFF )	{
		return -2;
	}

	for (i=1; i<ipx_num_packets; i++ )	{
		ipx_cancel_listen_for_packet(&packets[i].ecb);
	}

	// Close existing socket...
	memset(&regs,0,sizeof regs);
	regs.d.edx = ipx_socket;
	regs.d.ebx = 1;	// Close socket
	__dpmi_int( 0x7A, &regs );

	ipx_socket = new_ipx_socket;

	// Repost all listen requests on the new socket...	
	for (i=1; i<ipx_num_packets; i++ )	{
		packets[i].ecb.in_use = 0;
		packets[i].ecb.socket_id = ipx_socket;
		ipx_listen_for_packet(&packets[i].ecb);
	}

	packets[0].ecb.socket_id = ipx_socket;
	packets[0].ipx.destination.socket_id = ipx_socket;

	ipx_packetnum = 0;
	// init packet buffers.
	for (i=0; i<MAX_PACKETS; i++ )	{
		packet_buffers[i].packetnum = -1;
		packet_free_list[i] = i;
	}
	num_packets = 0;
	largest_packet_index = 0;

	return 0;
}

#else /* __USE_REAL_IPX_DOS__ */

/* Can't use the DOS-style IPX on this platform, so put in a dummy
 * driver. */

#include <stdlib.h>
#include <stdio.h>

#include "libnet.h"
#include "internal.h"

static int detect (void) { return NET_DETECT_NO; }
static void load_config (NET_DRIVER *drv, FILE *fp) { }

NET_DRIVER net_driver_ipx_dos = {
	"IPX from DOS", "Dummy driver", NET_CLASS_IPX,
	detect,
	NULL, NULL,
	NULL, NULL,
	NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL,
	load_config,
	NULL, NULL
};

#endif
