#ifndef _TYPES_H
#define _TYPES_H


//define a byte 
typedef signed char byte;

//define unsigned types;
typedef unsigned char ubyte;

/* Linux already has these definitions */
#ifndef __linux__
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
#endif

//define a boolean
typedef ubyte bool;


#define __pack__ __attribute__ ((packed))

/* Maybe we should decide whether to use the above typedefs or these */
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#endif

