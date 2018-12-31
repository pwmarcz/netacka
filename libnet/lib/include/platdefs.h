#if defined TARGET_RSXNTDJ

#define __USE_REAL_WSOCK_WIN__

#elif defined TARGET_DJGPP

#define __USE_REAL_WSOCK_DOS__
#define __USE_REAL_IPX_DOS__
#define __USE_REAL_SERIAL__
#define __USE_REAL_SERIAL_DOS__

#elif defined TARGET_MSVC

#define __USE_REAL_WSOCK_WIN__
#define __USE_REAL_IPX_WIN__
#define inline __inline

#elif defined TARGET_MINGW32

#define __USE_REAL_WSOCK_WIN__
#define __USE_REAL_IPX_WIN__

#elif defined TARGET_LINUX

#define __USE_REAL_SOCKS__
#define __USE_REAL_SERIAL__
#define __USE_REAL_SERIAL_LINUX__
#define __USE_REAL_IPX_LINUX__

#elif defined TARGET_BEOS

#define __USE_REAL_SERIAL__
#define __USE_REAL_SERIAL_BEOS__
#define __USE_REAL_BESOCKS__

#else /* Assume Unix anyway */

#define __USE_REAL_SOCKS__

#endif

