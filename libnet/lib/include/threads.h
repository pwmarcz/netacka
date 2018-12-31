/*----------------------------------------------------------------
 * threads.h - macros to help the library be thread-safe
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */


#ifndef libnet_included_file_threads_h
#define libnet_included_file_threads_h


/* Thread-safety function pointers */
extern void (* __libnet_internal__mutex_create) (void **mutex);
extern void (* __libnet_internal__mutex_destroy) (void *mutex);
extern void (* __libnet_internal__mutex_lock) (volatile void *mutex);
extern void (* __libnet_internal__mutex_unlock) (volatile void *mutex);



#ifdef NO_THREAD_SAFE


/* Don't care about thread-safety. */

#define MUTEX_DEFINE(id)
#define MUTEX_DEFINE_STATIC(id)
#define MUTEX_CLEAR(id)
#define MUTEX_LOCK(id)
#define MUTEX_UNLOCK(id)


#else


/* Be thread-safe. */

#define MUTEX_DEFINE(id)         void *id##_mutex
#define MUTEX_DEFINE_STATIC(id)  static void *id##_mutex
#define MUTEX_CREATE(id)         __libnet_internal__mutex_create (&id##_mutex)
#define MUTEX_DESTROY(id)        __libnet_internal__mutex_destroy (id##_mutex)
#define MUTEX_LOCK(id)           __libnet_internal__mutex_lock (id##_mutex)
#define MUTEX_UNLOCK(id)         __libnet_internal__mutex_unlock (id##_mutex)


#endif



#endif

