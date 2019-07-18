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

#if USE_SEMAPHORE
sem_t belegte_pufferplaetze;
sem_t freie_pufferplaetze;
#else
pthread_cond_t cond_p = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_c = PTHREAD_COND_INITIALIZER;
int errno;
#endif

pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

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

char dequeue(Queue *queue) {
    if (queue == NULL) {
        return '?';
    }

#if USE_SEMAPHORE
    sem_wait(&belegte_pufferplaetze);
#else
    while (getIsEmptyFlag(queue)) {
		pthread_mutex_lock(&buffer_mutex);
		pthread_cond_wait(&cond_c, &buffer_mutex);

	}
#endif
    if (queue->head->next == NULL) {
        return '?';
    }
    pthread_mutex_lock(&buffer_mutex);

    Node *returnNode = queue->head->next;
    queue->head->next = queue->head->next->next;
    char data = returnNode->data;

    free(returnNode);
    queue->size -=1;
    pthread_mutex_unlock(&buffer_mutex);
#if USE_SEMAPHORE
    sem_post(&freie_pufferplaetze);
#endif

    return data;
}

int getSize(Queue *queue) {
    if (queue != NULL) {
        return queue->size;
    } else {
        return ERROR;
    }
}

int enqueue(Queue *queue, char data) {
    if (queue == NULL) {
        return ERROR;
    }
#if USE_SEMAPHORE
    sem_wait(&freie_pufferplaetze);
#else
    while(getIsFullFlag(queue)){
		pthread_mutex_lock(&buffer_mutex);
		pthread_cond_wait(&cond_p, &buffer_mutex);
	}
#endif
    pthread_mutex_lock(&buffer_mutex);
    if (queue->size >= SIZE) {
        return ERROR;
    }

    Node *newNode = malloc(sizeof(*newNode));
    newNode->data = data;
    newNode->next = NULL;

    Node *worker = queue->head;
    while (worker->next != NULL) {
        worker = worker->next;
    }
    worker->next = newNode;
    queue->size += 1;
    pthread_mutex_unlock(&buffer_mutex);
#if USE_SEMAPHORE
    sem_post(&belegte_pufferplaetze);
#else
    pthread_cond_signal(&cond_c);
#endif
    return SUCCESS;
}

int printQueue(Queue *queue) {
    if (queue == NULL) {
        return ERROR;
    }

    pthread_mutex_lock(&buffer_mutex);

    Node *worker = queue->head;
    printf("Data:");
    while (worker->next != NULL) {
        worker = worker->next;
        printf("%c   ", worker->data);
    }
    printf("\n");

    pthread_mutex_unlock(&buffer_mutex);
    return 0;
}
