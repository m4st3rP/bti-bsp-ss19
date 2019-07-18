/*
 * queue1.c
 *
 *  Created on: 15.05.2019
 *      Author: Tobias Ranfft
 */

#include "definitions.h"
#include "queue.h"
#include <stddef.h> // import for NULL
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

#define ERROR -1
#define SUCCESS 0
#define SIZE 10

sem_t belegte_pufferplaetze;
sem_t freie_pufferplaetze;

pthread_cond_t cond_p = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_c = PTHREAD_COND_INITIALIZER;

pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

int errno;
char data;
typedef struct node {
	Node *next;
	char data;
} Node;

typedef struct queue {
	Node *head;
	int size;
} queue;

Queue *newQueue(void) {
	Queue *queue = malloc(sizeof(*queue));
	Node *node = malloc(sizeof(*node)); // create stopper node
	queue->head = node;
	queue->size = 0;
	return queue;
}

bool getIsFullFlag(Queue *queue) {
	if (queue->size == SIZE) {
		return true;
	} else {
		return false;
	}
}

bool getIsEmptyFlag(Queue *queue) {
	if (queue->size == 0) {
		return true;
	} else {
		return false;
	}
}

int getSize(Queue *queue) {
	if (queue != NULL) {
		return queue->size;
	} else {
		return ERROR;
	}
}

// All CleanUpHAndlers:
static void cleanupMutex(void* arg) {
	printf("cleanup Mutex called\n");
	pthread_mutex_t* toUnlock = (pthread_mutex_t*) arg;
	errno = pthread_mutex_unlock(toUnlock);
	if (errno != 0) {
		printf("fehler im cleanup coudlnt unlock, NR: %d\n", errno);
	}
}

// struct for queueing arguments
typedef struct cleanUpArg {
	char queued;
	Queue* queue;
	bool dequeueing;
} CleanUpArg;

// codefür dequeue:
//Node *returnNode = queue->head->next;
//queue->head->next = queue->head->next->next;
//data = returnNode->data;
//free(returnNode);
//queue->size -= 1;


//Code für enqueue:
//Node *newNode = malloc(sizeof(*newNode));
//newNode->data = input;
//newNode->next = NULL;
//Node *worker = queue->head;
//while (worker->next != NULL) {
//	worker = worker->next;
//}
//worker->next = newNode;
//queue->size += 1;

static void cleanUpqueue(void* arg){
	
	CleanUpArg* args = (CleanUpArg*) arg;
	if(args->dequeueing){
        printf("<-----------CleanupDequeue called\n");
		Node *newNode = malloc(sizeof(*newNode));
		newNode->data = args->queued;
		newNode->next = args->queue->head->next;
		args->queue->head->next = newNode;
		args->queue->size += 1;
        printf("<----------CleanUp Dequeue finished");
	}else{
        printf("<-----------Clean Up Queueing called\n");
		Node* worker = args->queue->head;
		
        while(worker->next != NULL){
			worker = worker->next;
		}
		//free(worker);
		args->queue->size -= 1;
	}

}

//printing the data of a queue
int printQueue(Queue *queue) {
	if (queue == NULL) {
		return ERROR;
	}
	void* argument = (void*) &buffer_mutex;
	pthread_cleanup_push(cleanupMutex, argument)
				;
				errno = pthread_mutex_lock(&buffer_mutex);
				if (errno != 0) {
					printf("Fehler bufferMutex lock in print Queue");
				}
				Node *worker = queue->head;
				printf("Data:");
				while (worker->next != NULL) {
					worker = worker->next;
					printf("%c   ", worker->data);
				}
				printf("\n");
				printf("%d\n", getSize(queue));
				errno = pthread_mutex_unlock(&buffer_mutex);
				if (errno != 0) {
					printf("Fehler bufferMutex unlock in print Queue");
				}
				pthread_cleanup_pop(0);
	return 0;

}

int enqueue(Queue *queue, char input) {
	void* argument = (void*) &buffer_mutex;
	CleanUpArg queueArgs;
	CleanUpArg* queueArg = &queueArgs;
	queueArg->dequeueing = false;
	queueArg->queue = queue;
	queueArg->queued = input;
#if USE_SEMAPHORE
	sem_wait(&freie_pufferplaetze);
#endif
	
	pthread_cleanup_push(cleanupMutex, argument)
		;
		errno = pthread_mutex_lock(&buffer_mutex);
		if(errno != 0){
			printf("fehler mutex lock enqueue %d", errno);
		}
#if USE_CONDITIONVARS
		while (getIsFullFlag(queue)) {
			errno = pthread_cond_wait(&cond_p, &buffer_mutex);
			if (errno != 0) {
				printf("Fehler in Enqueue condWait Nr: %d", errno);
			}
		}
#endif
		if (queue->size >= SIZE) {
			return ERROR;
		}
        pthread_cleanup_push(cleanUpqueue, (void*)queueArg);
		Node *newNode = malloc(sizeof(*newNode));
		newNode->data = input;
		newNode->next = NULL;
		Node *worker = queue->head;
		while (worker->next != NULL) {
			worker = worker->next;
		}
		worker->next = newNode;
		queue->size += 1;
		//ADD Routine to reset the Queueing push on cleanup
		//pop one more time

#if USE_CONDITIONVARS
		pthread_cond_signal(&cond_c);
#endif

		errno = pthread_mutex_unlock(&buffer_mutex);
		

		if(errno != 0){
			printf("Fehleer in enqueue coudlnt unlock buffer mutex %d", errno);
		}
#if USE_SEMAPHORE
		sem_post(&belegte_pufferplaetze);
#endif
		pthread_cleanup_pop(0);
        pthread_cleanup_pop(0);
	return SUCCESS;
}

char dequeue(Queue *queue) {
	if (queue == NULL) {
		return '?';
	}
	void* argument = (void*) &buffer_mutex;


#if USE_SEMAPHORE
	sem_wait(&belegte_pufferplaetze);
#endif

	pthread_cleanup_push(cleanupMutex, argument);
	pthread_mutex_lock(&buffer_mutex);

#if USE_CONDITIONVARS
	while (getIsEmptyFlag(queue)) {
		pthread_cond_wait(&cond_c, &buffer_mutex);
	}
#endif
	Node *returnNode = queue->head->next;
	queue->head->next = queue->head->next->next;
	data = returnNode->data;
	free(returnNode);
	queue->size -= 1;

	//Add Routine to resset Dequeueing push on cleanup
	//Pop one more time


#if USE_CONDITIONVARS
	pthread_cond_signal(&cond_p);
#endif

	pthread_mutex_unlock(&buffer_mutex);
	pthread_cleanup_pop(0);
#if USE_SEMAPHORE
	sem_post(&freie_pufferplaetze);
#endif


	return data;
}

