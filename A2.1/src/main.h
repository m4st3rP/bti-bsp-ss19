/*
 * workerThread.h
 *
 *  Created on: 27.04.2019
 *      Author: tranfft
 */

#ifndef SRC_WORKERTHREAD_H_
#define SRC_WORKERTHREAD_H_

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include "queue.h"


extern int errno;
extern sem_t belegte_pufferplaetze;
extern sem_t freie_pufferplaetze;
extern pthread_mutex_t buffer_mutex;





#endif /* SRC_WORKERTHREAD_H_ */
