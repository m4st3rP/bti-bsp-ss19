/*
 * workerThread.c
 *
 *  Created on: 27.04.2019
 *      Author: tranfft
 */
#include "definitions.h"
#include "main.h"
#include <pthread.h>
#include "semaphore.h"
#include <stdlib.h>
#include <stdio.h>
#include "queue.h"
#include <unistd.h>
#include <stdbool.h>
#include "taskqueue.h"
#define ALPHABETLENGTH 25
#define QUEUEZIZEMAX 10
#define SLEEPTIMEPRODUCER 3
#define SLEEPTIMECONSUMER 2
#define WORKERSIZE 5
typedef struct arguments {
	int id;
	char startChar;
	pthread_mutex_t* pause_mut;
	Queue* queue;
	mqd_t* taskqueue;
} Arguments;

static pthread_t producers[WORKERSIZE];
static pthread_t consumers[WORKERSIZE];
pthread_t controller1, producerGenerator1, consumerGenerator1;

pthread_mutex_t proGenMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t conGenMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t taskQueueMut = PTHREAD_MUTEX_INITIALIZER;

#if USE_SEMAPHORE
sem_t belege_pufferplaetze;
sem_t freie_pufferplaetze;
#else
pthread_cond_t cond_p;
pthread_cond_t cond_c;
#endif

Arguments argsp1;
Arguments argcon1;
int errno;

void *producer(void* arguments) {
	Arguments *args = arguments;
	char input = args->startChar;
	enqueue(args->queue, input);
	printQueue(args->queue);
	printf("last produced char was %c\n", input);
	sleep(SLEEPTIMEPRODUCER);

	return NULL;
}
static void cleanupMutex(void* arg) {
	printf("cleanupMutex called\n");
	pthread_mutex_t* toUnlock = (pthread_mutex_t*) arg;
	errno = pthread_mutex_unlock(toUnlock);
	if (errno != 0) {
		printf("fehler queue.c line 36, NR: %d\n", errno);
	}
}




void *consumer(void *arguments) {
	Arguments *args = arguments;
	char firstInChar = dequeue(args->queue);
	printf("\n");
	printQueue(args->queue);
	printf("last consumed char was %c\n", firstInChar);
	sleep(SLEEPTIMECONSUMER);

	return NULL;
}

//TODO workerThread implementieren
void *worker(void *workerArgus) {
	Arguments* workerArgs = (Arguments*) workerArgus;
	mqd_t* workerTaskQueue = workerArgs->taskqueue;
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	while (true) {
		if (errno != 0) {
			printf(
					"Fehler bei Mutex Lock von worker with ID: %d -- errno number %d",
					workerArgs->id, errno);
		}
		Task todo = receiveFromTaskQueue(*workerTaskQueue);
		if (errno != 0) {
			printf(
					"Fehler bei Mutex unlock von worker with id %d -- errno number: %d",
					workerArgs->id, errno);
		}
		(*todo.routineForTask)(todo.arg);

	}
	return NULL;
}

void *produceGenerator(void* generatorArgs) {
	Arguments* producerTQ = (Arguments*) generatorArgs;
	mqd_t *producerTaskQ = producerTQ->taskqueue;
	char inputChar = 'a';
	int offset = 0;
	bool running = true;
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	while (running) {
		void* arguments = (void*) &proGenMut;
		pthread_cleanup_push(cleanupMutex, arguments)
					;
					errno = pthread_mutex_lock(&proGenMut);
					if (errno != 0) {
						printf(
								"Fehler beim lock von proGen mutex -- Fehler nummer: %d\n",
								errno);
					}else{
						printf("ProGenMut LOCKED in Generator\n");
					}
					errno = pthread_mutex_unlock(&proGenMut);
					if(errno == 0){
						printf("ProGenMut UNLOCKED in Generator\n");
					}else{
						printf("Couldnt unlock ProGenMut  %d\n", errno);
					}
					pthread_cleanup_pop(0);


		Task producertask;
		producertask.routineForTask = (void*) &producer;
		argsp1.startChar = inputChar + offset;
		producertask.arg = (void*) &argsp1;
		sendToTaskQueue(*producerTaskQ, producertask, 20, true);
		if (offset < ALPHABETLENGTH) {
			offset++;
		} else {
			offset = 0;
		}
		sleep(1);

	}
	return NULL;
}

void *consumeGenerator(void *generatorArgs) {
	Arguments* consumerTQ = (Arguments*) generatorArgs;
	mqd_t *consumerTaskQ = consumerTQ->taskqueue;
	char inputChar = 'a';
	int offset = 0;
	bool running = true;
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	while (running) {
		void* arguments = (void*) &proGenMut;
		pthread_cleanup_push(cleanupMutex, arguments)
					;

					errno = pthread_mutex_lock(&conGenMut);
					if (errno != 0) {
						printf(
								"Fehler beim lock von conGenMut mutex -- Fehler nummer: %d",
								errno);
					}
					errno = pthread_mutex_unlock(&conGenMut);
					if (errno != 0){
						printf("couldnt unlock conGenMut in consumerGenerator");
					}
					pthread_cleanup_pop(0);
		if (errno != 0) {
			printf("Fehler beim unlock von conGenMut mutex -- Fehler nummer: %d",
					errno);
		}

		Task consumerTask;
		consumerTask.routineForTask = (void*) &consumer;
		argsp1.startChar = inputChar + offset;
		consumerTask.arg = (void*) &argsp1;
		errno = pthread_mutex_lock(&taskQueueMut);
		if (errno != 0) {
			printf("Couldnt lock taskQueueMut -- errno number: %d", errno);
		}
		sendToTaskQueue(*consumerTaskQ, consumerTask, 20, true);
		errno = pthread_mutex_unlock(&taskQueueMut);

		if (errno != 0) {
			printf("Couldnt unlock taskQueueMut -- errno number: %d", errno);
		}
		if (offset < ALPHABETLENGTH) {
			offset++;
		} else {
			offset = 0;
		}

		sleep(1);
	}
	return NULL;
}

void *controller(void* notUsed) {
	bool producerGeneratorRunning = true;
	bool consumerGeneratorRunning = true;
	Queue* queue = (Queue*) notUsed;
	while (true) {
		char c = getchar();

		switch (c) {
		case 'p':
		case 'P':

			if (producerGeneratorRunning) {

				errno = pthread_mutex_lock(&proGenMut);
				if (errno != 0) {
					printf(
							"Fehler Mutex Lock Producer Generator innerhalb des controller -- errno number: %d\n",
							errno);
				} else {
					printf("proGenMut LOCKED\n");
				}
				producerGeneratorRunning = false;
				printf("Stopped Producer Generator \n");

			} else if (!producerGeneratorRunning) {
				errno = pthread_mutex_unlock(&proGenMut);
				if (errno != 0) {
					printf(
							"Fehler Mutex Unlock producerGenerator1 innerhalb des controller -- errno number: %d\n",
							errno);
				} else {
					printf("proGenMut UNLOCKED\n");
				}
				producerGeneratorRunning = true;
				printf("Started Producer Generator\n");

			}
			break;
		case 'c':
		case 'C':

			if (!consumerGeneratorRunning) {
				errno = pthread_mutex_unlock(&conGenMut);
				if (errno != 0) {
					printf(
							"Fehler Mutex Unlock consumer innerhalb des controller -- errno number: %d\n",
							errno);
				} else {
					printf("conGenMut UNLOCKED\n");
				}
				consumerGeneratorRunning = true;
				printf("Started Consumer\n");
			} else if (consumerGeneratorRunning) {
				errno = pthread_mutex_lock(&conGenMut);
				if (errno != 0) {
					printf(
							"Fehler Mutex Lock Consumer1 innerhalb des controller -- errno number: %d\n",
							errno);
				} else {
					printf("conGenMut LOCKED\n");
				}
				consumerGeneratorRunning = false;
				printf("Stopped Consumer\n");
			}
			break;

		case 'q':
		case 'Q':
			printf("\n!!!\n!!!\n!!!\n");
			printf("Starting ending");
			printQueue(queue);
			printf("cancle producer Generator\n");
			if (producerGeneratorRunning) {
				errno = pthread_mutex_lock(&proGenMut);
				if (errno != 0) {
					printf("fehler beim MUTEX LOCK DIREKT VOR DEM CANCEL");
				}else{
					printf("Locked Mutex proGenMut to sen cancel \n");
				}
			}

			errno = pthread_cancel(producerGenerator1);
			if (errno != 0) {
				printf(
						"Fehler  beim Cancel von producerGenerator1 innerhalb des controller -- errno number: %d\n",
						errno);
			} else {
				printf("Sended request to end producerGenerator\n");
			}

			errno = pthread_mutex_unlock(&proGenMut);
			if (errno != 0) {
				printf(
						"fehler bei mutex unlock producer generator while canceling %d",
						errno);
			} else {
				printf("proGenMut UNLOCKED\n");
			}
			if (consumerGeneratorRunning) {
				errno = pthread_mutex_lock(&conGenMut);
				if (errno != 0) {
					printf("fehler beim MUTEX LOCK DIREKT VOR DEM CANCEL");
				}
			}

			printf("cancle consumer Generator\n");
			errno = pthread_cancel(consumerGenerator1);
			if (errno != 0) {
				printf(
						"Fehler  beim Cancel von Consumer1 innerhalb des controller -- errno number: %d\n",
						errno);
			}

			errno = pthread_mutex_unlock(&conGenMut);
			if (errno != 0) {
				printf(
						"Fehler  beim unlock mutex von producerGenerator1 innerhalb des controller(q) -- errno number: %d\n",
						errno);
			} else {
				printf("conGenMut UNLOCKED\n");
			}

			errno = pthread_join(consumerGenerator1, NULL);
			if (errno != 0) {
				printf("consumer generator couoldnt join\n");
			}else{
				printf("consumer generator joined\n");
			}
			errno = pthread_join(producerGenerator1, NULL);
			if (errno != 0) {
				printf("producer generator couldnt join\n");
			}else{
				printf("producer generator joined\n");
			}
			printf("Cancelled every thread\n");
			return NULL;
			break;

		case 'h':
			printf(
					" Press p or P to start/stopp producerGenerator1 \n Press c or C to start/stopp consumer \n Press q or Q to quit and exit \n Press h for help");
			break;
		case '\n':
			break;
		default:
			printf("Invalid input! Press h to get help!\n");
			break;
		}
	}
}

int main() {
// initialize the semaphores
#if USE_SEMAPHORE
	printf("SEMAPHORES USED\n");
	errno = sem_init(&belege_pufferplaetze, 0, 0);
	if (errno != 0) {
		printf(
				"Fehler  beim semaphore init belegte Plätze -- errno number: %d\n",
				errno);
	}
	errno = sem_init(&freie_pufferplaetze, 0, 10);
	if (errno != 0) {
		printf("Fehler  beim semaphore init freie Plätze -- errno number: %d\n",
				errno);
	}
#else
	printf("CONDITION VARS USED\n");
#endif

	Queue *queue = newQueue();
	const char *name = "/proname";

	const char *name2 = "/consname";
	const unsigned int size = 50;
	mqd_t prodTQ = createTaskQueue(name, size);
	mqd_t consTQ = createTaskQueue(name2, size);
	mqd_t *prodTQptr = &prodTQ;
	mqd_t *conTQptr = &consTQ;
// fill argument structs for producer threads
	argsp1.id = 1;
	argsp1.startChar = 'a';
	argsp1.pause_mut = &proGenMut;
	argsp1.queue = queue;
	argsp1.taskqueue = prodTQptr;

	argcon1.id = 2;
	argcon1.startChar = '\0';
	argcon1.pause_mut = &conGenMut;
	argcon1.queue = queue;
	argcon1.taskqueue = conTQptr;

// create controller thread
	printf("Creating Controller\n");
	errno = pthread_create(&controller1, NULL, controller, (void*) queue);
	if (errno != 0) {
		printf(
				"Fehler  beim Create des Controllers in main -- errno number: %d\n",
				errno);
	}

//CREATING GENERATORS
	printf("Creating producerGenerators\n");
	errno = pthread_create(&producerGenerator1, NULL, &produceGenerator,
			(void*) &argsp1);
	if (errno != 0) {
		printf(
				"Fehler Pthread Create von producerGenerator1 innerhalb des controller -- errno number: %d\n",
				errno);
	}
	printf("Creating Consumer Generators\n");
	errno = pthread_create(&consumerGenerator1, NULL, &consumeGenerator,
			(void *) &argcon1);
	if (errno != 0) {
		printf(

				"Fehler Pthread Create von consumerGenerator1 innerhalb des controller -- errno number: %d\n",
				errno);
	}

//CREATING Producers

	for (int i = 0; i < WORKERSIZE; i++) {

		errno = pthread_create(&producers[i], NULL, worker, (void*) &argsp1);
		if (errno != 0) {
			printf(
					"Fehler  beim Create des workerp1 in main -- errno number: %d\n",
					errno);
		}
	}

//CREATING consumers
	for (int i = 0; i < WORKERSIZE; i++) {

		errno = pthread_create(&consumers[i], NULL, worker, (void*) &argcon1);
		if (errno != 0) {
			printf(
					"Fehler  beim Create des workerp1 in main -- errno number: %d\n",
					errno);
		}
	}

// wait for termination of controller thread-------------------------------------------------
	errno = pthread_join(controller1, NULL);
	if (errno != 0) {
		printf("Fehler bei pthread_join Controller -- errno number: %d\n",
				errno);
	}

	for (int i = 0; i < WORKERSIZE; i++) {
		errno = pthread_cancel(producers[i]);
		if (errno != 0) {
			printf(
					"Fehler  beim Cancel von workerp1 innerhalb des controller -- errno number: %d\n",
					errno);
		}
		errno = pthread_cancel(consumers[i]);
		if (errno != 0) {
			printf(
					"Fehler  beim Cancel von workerp1 innerhalb des controller -- errno number: %d\n",
					errno);
		}
	}

	for (int i = 0; i < WORKERSIZE; i++) {
		errno = pthread_join(producers[i], NULL);
		if (errno != 0) {
			printf("Fehler bei pthread_join Controller -- errno number: %d\n",
					errno);
		}
	}
	for (int i = 0; i < WORKERSIZE; i++) {
		errno = pthread_join(consumers[i], NULL);
		if (errno != 0) {
			printf("Fehler bei pthread_join Controller -- errno number: %d\n",
					errno);
		}
	}
	printf("End of joins\n");
	printQueue(queue);
// clear resources
#if USE_SEMAPHORE
	errno = sem_destroy(&freie_pufferplaetze);
	if (errno != 0) {
		printf(
				"Fehler  beim semaphore destroy freie Plätze -- errno number: %d \n",
				errno);
	}else{
		printf("Destroyed freie Paetze\n");
	}
	errno = sem_destroy(&belegte_pufferplaetze);
	if (errno != 0) {
		printf(
				"Fehler  beim semaphore destroy belegte Plätze -- errno number: %d\n",
				errno);
	}else{
		printf("Destroyed Semaphore Belegte Plaetze\n");
	}
#else
    errno = pthread_cond_destroy(&cond_p);
    if (errno != 0) {
        printf(" Fehler beim zerstören von cond_p \n");
    }else{
        printf("destroyed cond_p\n");
    }
    errno = pthread_cond_destroy(&cond_c);
    if (errno != 0) {
        printf(" Fehler beim zerstören von cond_c\n");
    }else{
        printf("Destroyed cond_c\n");
    }
#endif
	errno = pthread_mutex_destroy(&proGenMut);
	if (errno != 0) {
		printf("Fehler  beim mutex destroy proGenMut -- errno number: %d\n", errno);
	} else {
		printf("destroyed proGenMut mutex\n");
	}

	errno = pthread_mutex_destroy(&conGenMut);
	if (errno != 0) {
		printf("Fehler  beim mutex destroy conGenMut -- errno number: %d\n", errno);
	} else {
		printf("destroyed conGenMut mutex\n");
	}
	errno = pthread_mutex_destroy(&buffer_mutex);
	if (errno != 0) {
		printf("Fehler  beim mutex destroy buffer_mutex -- errno number: %d\n",
				errno);
	} else {
		printf("destroyed buffer mutex\n");
	}

	closeTaskQueue(prodTQ);

	closeTaskQueue(consTQ);

	destroyTaskQueue(name);

	destroyTaskQueue(name2);
	printf("End of Main\n");

	return 0;
}
