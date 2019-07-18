/*
 * trans.h -- definitions for the char module
 *
 * The code of this file is based on the code examples and the 
 * content of the book "Linux Device * Drivers" by Alessandro 
 * Rubini and Jonathan Corbet, published * by O'Reilly & Associates.   
 *
 * No warranty is attached; we cannot take responsibility 
 * for errors or fitness for use.
 *
 * Franz Korf HAW Hamburg
 */

#ifndef _TRANS_H_
#define _TRANS_H_

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

#ifndef init_MUTEX
#define init_MUTEX(mutex) sema_init(mutex, 1)
#endif	

/*
 * Macros to help debugging
 */

#undef PDEBUG             /* undef it, just in case */
#ifdef TRANS_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_ALERT "trans: " fmt, ## args) 
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#ifndef TRANS_MINOR
#define TRANS_MINOR 0   /* dynamic major by default */
#endif

#define TRANS_BUF_SIZE  64 /* Buffer size for read and write buffer */

#endif /* _TRANS_H_ */
