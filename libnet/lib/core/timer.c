/*----------------------------------------------------------------
 *   timer.c -- default millisecond timer function
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

/* Why is this here?  Why don't I just use `clock'?  Well, I
 * did, but then found that under Linux in particular `clock'
 * does not indicate real-life time, it indicates the process's
 * time only.  When `rdmtest' sat waiting for input, the value
 * returned by `clock' wasn't increasing.  I thought this was a
 * bug in the RDM code, when in fact it's just a
 * misunderstanding.  Also, this way lets platforms use whatever
 * their most accurate timing method is, to get better
 * granularity than the 1 second provided by `time'.  Plus the
 * user can override this function.  Everybody wins.
 *
 * It should return a time in milliseconds.
 */

#include <libnet.h>
#include "timer.h"

unsigned int (* __libnet_timer_func) (void);
static unsigned int default_timer_func (void);

void net_set_timer_func (
	unsigned int (*timer) (void)
)
{
	if (timer)
		__libnet_timer_func = timer;
	else
		__libnet_timer_func = default_timer_func;
}


#if defined TARGET_LINUX

#include <sys/time.h>
#include <unistd.h>
#include "threads.h"

static unsigned int default_timer_func (void)
{
	static struct timeval start_time;
	static int virgin = 1;
	MUTEX_DEFINE_STATIC(timer);

	struct timeval current_time;
	unsigned int retval;
	
	if (virgin) {
		virgin = 0;
		gettimeofday (&start_time, NULL);
		MUTEX_CREATE(timer);
	}
	
	MUTEX_LOCK(timer);
	
	gettimeofday (&current_time, NULL);
	current_time.tv_sec -= start_time.tv_sec;
	current_time.tv_usec -= start_time.tv_usec;
	
	retval = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;

	MUTEX_UNLOCK(timer);

	return retval;
}

#elif defined TARGET_MSVC || defined TARGET_MINGW32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static unsigned int default_timer_func (void)
{
	return GetTickCount();
}

#else

#include <time.h>
#include <stdlib.h>   /* for MSVC (?) */

static unsigned int default_timer_func (void)
{
	return (unsigned int)(clock() * 1000.0 / CLOCKS_PER_SEC);
}

#endif

