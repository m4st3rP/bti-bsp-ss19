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
#include <errno.h>

#define ALPHABETLENGTH 26
#define SLEEPTIMEPRODUCER 3
#define SLEEPTIMECONSUMER 2

typedef struct arguments {
    int id;
    char startChar;
    pthread_mutex_t *pause_mut;
    Queue *queue;
} Arguments;

pthread_mutex_t pro1_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pro2_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t con1_m = PTHREAD_MUTEX_INITIALIZER;

#if USE_SEMAPHORE
sem_t belege_pufferplaetze;
sem_t freie_pufferplaetze;
#else
pthread_cond_t cond_p;
pthread_cond_t cond_c;
#endif

Arguments argsp1;
Arguments argsp2;
Arguments argcon1;

void *producer(void *arguments) {
    Arguments *args = arguments;
    char input = args->startChar;
    pthread_mutex_t *pause = args->pause_mut;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    int offset = 0;
    while (true) {
        errno = pthread_mutex_lock(pause);
        if (errno != 0) {
            printf(
                    "Fehler Mutex Lock innerhalb des producer %d  -- errno number: %d\n",
                    args->id, errno);
        }
        errno = pthread_mutex_unlock(pause);
        if (errno != 0) {
            printf(
                    "Fehler Mutex Unock innerhalb des producer %d  -- errno number: %d\n",
                    args->id, errno);
        }
        char actualChar = input + offset;

        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        enqueue(args->queue, actualChar);
        printQueue(args->queue);
        printf("last produced char was %c\n", actualChar);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        if (offset < ALPHABETLENGTH) {
            offset++;
        } else {
            offset = 0;
        }
        sleep(SLEEPTIMEPRODUCER);
    }
    return NULL;
}

void *consumer(void *arguments) {
    Arguments *args = arguments;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    char firstInChar = '\0';
    pthread_mutex_t *pause = args->pause_mut;
    while (true) {
        errno = pthread_mutex_lock(pause);
        if (errno != 0) {
            printf(
                    "Fehler Mutex Lock innerhalb des consumer -- errno number: %d\n",
                    errno);
        }
        printf("locked con1\n");
        errno = pthread_mutex_unlock(pause);
        if (errno != 0) {
            printf(
                    "Fehler Mutex Unlock innerhalb des consumer -- errno number: %d\n",
                    errno);
        }
        printf("unlocked con1\n");
        firstInChar = dequeue(args->queue);
        printf("\n");
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        printQueue(args->queue);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        printf("last consumed char was %c\n", firstInChar);
        sleep(SLEEPTIMECONSUMER);
    }
    return NULL;
}

void *controller(void *notUsed) {
    bool producer1Exists = false;
    bool producer1Running = false;
    bool producer2Exists = false;
    bool producer2Running = false;
    bool consumerExists = false;
    bool consumerRunning = false;
    pthread_t producer1, producer2, consumer1;
    argsp1.startChar = 'a';
    argsp1.pause_mut = &pro1_m;
    while (true) {
        char c = getchar();

        switch (c) {
            case '1':
                if (!producer1Exists && !producer1Running) {
                    errno = pthread_create(&producer1, NULL, producer,
                                           (void *) &argsp1);
                    if (errno != 0) {
                        printf(
                                "Fehler Pthread Create von producer1 innerhalb des controller -- errno number: %d\n",
                                errno);
                    }
                    producer1Running = true;
                    producer1Exists = true;
                    printf("Created and started Producer 1\n");
                } else if (producer1Exists && !producer1Running) {
                    errno = pthread_mutex_unlock(&pro1_m);
                    if (errno != 0) {
                        printf(
                                "Fehler Mutex Unock producer1 innerhalb des controller -- errno number: %d\n",
                                errno);
                    }
                    producer1Running = true;
                    printf("Started Producer 1\n");
                } else if (producer1Exists && producer1Running) {
                    errno = pthread_mutex_lock(&pro1_m);
                    if (errno != 0) {
                        printf(
                                "Fehler Mutex Lock Producer1 innerhalb des controller -- errno number: %d\n",
                                errno);
                    }
                    producer1Running = false;
                    printf("Stopped Producer 1\n");
                }
                break;
            case '2':
                if (!producer2Exists && !producer2Running) {
                    errno = pthread_create(&producer2, NULL, producer,
                                           (void *) &argsp2);
                    if (errno != 0) {
                        printf(
                                "Fehler Pthread Create von producer2 innerhalb des controller -- errno number: %d\n",
                                errno);
                    }
                    producer2Running = true;
                    producer2Exists = true;
                    printf("Created and started Producer 2\n");
                } else if (producer2Exists && !producer2Running) {
                    errno = pthread_mutex_unlock(&pro2_m);
                    if (errno != 0) {
                        printf(
                                "Fehler Mutex Unock producer2 innerhalb des controller -- errno number: %d\n",
                                errno);
                    }
                    producer2Running = true;
                    printf("Started Producer 2\n");
                } else if (producer2Exists && producer2Running) {
                    errno = pthread_mutex_lock(&pro2_m);
                    if (errno != 0) {
                        printf(
                                "Fehler Mutex Lock Producer1 innerhalb des controller -- errno number: %d\n",
                                errno);
                    }
                    producer2Running = false;
                    printf("Stopped Producer 2\n");
                }

                break;

            case 'c':
            case 'C':
                if (!consumerExists && !consumerRunning) {
                    errno = pthread_create(&consumer1, NULL, consumer,
                                           (void *) &argcon1);

                    if (errno != 0) {
                        printf(

                                "Fehler Pthread Create von consumer1 innerhalb des controller -- errno number: %d\n",
                                errno);
                    }
                    consumerRunning = true;
                    consumerExists = true;
                    printf("Created and started Consumer\n");
                } else if (consumerExists && !consumerRunning) {
                    errno = pthread_mutex_unlock(&con1_m);
                    if (errno != 0) {
                        printf(
                                "Fehler Mutex Unock consumer innerhalb des controller -- errno number: %d\n",
                                errno);
                    }
                    consumerRunning = true;
                    printf("Started Consumer\n");
                } else if (consumerExists && consumerRunning) {
                    errno = pthread_mutex_lock(&con1_m);
                    if (errno != 0) {
                        printf(
                                "Fehler Mutex Lock Consumer1 innerhalb des controller -- errno number: %d\n",
                                errno);
                    }
                    consumerRunning = false;
                    printf("Stopped Consumer\n");
                }
                break;

            case 'q':
            case 'Q':
                if (producer1Running == 0 && producer1Exists == 1) {
                    errno = pthread_cancel(producer1);
                    if (errno != 0) {
                        printf(
                                "Fehler  beim Cancel von producer1 innerhalb des controller -- errno number: %d\n",
                                errno);
                    }
                    errno = pthread_mutex_unlock(&pro1_m);
                    if (errno != 0) {
                        printf(
                                "Fehler  beim unlock mutex von producer1 innerhalb des controller(q) -- errno number: %d\n",
                                errno);
                    }
                }
                if (producer2Running == 0 && producer2Exists == 1) {
                    errno = pthread_cancel(producer2);
                    if (errno != 0) {
                        printf(
                                "Fehler  beim Cancel von producer2 innerhalb des controller -- errno number: %d\n",
                                errno);
                    }
                    errno = pthread_mutex_unlock(&pro2_m);
                    if (errno != 0) {
                        printf(
                                "Fehler  beim unlock mutex von producer2 innerhalb des controller(q) -- errno number: %d\n",
                                errno);
                    }
                }
                if (consumerRunning == 0 && consumerExists) {
                    errno = pthread_cancel(consumer1);
                    if (errno != 0) {
                        printf(
                                "Fehler  beim Cancel von Consumer1 innerhalb des controller -- errno number: %d\n",
                                errno);
                    }

                    errno = pthread_mutex_unlock(&con1_m);
                    if (errno != 0) {
                        printf(
                                "Fehler  beim unlock mutex von consumer innerhalb des controller(q) -- errno number: %d\n",
                                errno);
                    }
                }
                printf("Cancelled every thread\n");
                return NULL;
                break;

            case 'h':
                printf(
                        " Press 1 to start/stopp producer1 \n Press 2 to start/stopp producer2 \n Press c or C to start/stopp consumer \n Press q or Q to quit and exit \n Press h for help");
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
#endif

    Queue *queue = newQueue();

// fill argument structs for producer threads
    argsp1.id = 1;
    argsp1.startChar = 'a';
    argsp1.pause_mut = &pro1_m;
    argsp1.queue = queue;

    argsp2.id = 2;
    argsp2.startChar = 'A';
    argsp2.pause_mut = &pro2_m;
    argsp2.queue = queue;

    argcon1.id = 3;
    argcon1.startChar = '\0';
    argcon1.pause_mut = &con1_m;
    argcon1.queue = queue;

// create controller thread
    pthread_t controller1;
    errno = pthread_create(&controller1, NULL, controller, NULL);
    if (errno != 0) {
        printf(
                "Fehler  beim Create des Controllers in main -- errno number: %d\n",
                errno);
    }
    errno = pthread_join(controller1, NULL); // wait for termination of controller thread
    if (errno != 0) {
        printf("Fehler bei pthread_join Controller -- errno number: %d\n",
               errno);
    }

// clear resources
#if USE_SEMAPHORE
    errno = sem_destroy(&freie_pufferplaetze);
    if (errno != 0) {
        printf(
                "Fehler  beim semaphore destroy freie Plätze -- errno number: %d \n",
                errno);
    }
    errno = sem_destroy(&belegte_pufferplaetze);
    if (errno != 0) {
        printf(
                "Fehler  beim semaphore destroy belegte Plätze -- errno number: %d\n",
                errno);
    }
#else
    errno = pthread_cond_destroy(&cond_p);
    if (errno != 0) {
        printf(" Fehler beim zerstören von cond_p ");
    }
    errno = pthread_cond_destroy(&cond_c);
    if (errno != 0) {
        printf(" Fehler beim zerstören von cond_c");
    }
#endif
    errno = pthread_mutex_destroy(&pro1_m);
    if (errno != 0) {
        printf("Fehler  beim mutex destroy pro1 -- errno number: %d\n", errno);
    }
    errno = pthread_mutex_destroy(&pro2_m);
    if (errno != 0) {
        printf("Fehler  beim mutex destroy pro2 -- errno number: %d\n", errno);
    }
    errno = pthread_mutex_destroy(&con1_m);
    if (errno != 0) {
        printf("Fehler  beim mutex destroy con1 -- errno number: %d\n", errno);
    }
    errno = pthread_mutex_destroy(&buffer_mutex);
    if (errno != 0) {
        printf("Fehler  beim mutex destroy buffer_mutex -- errno number: %d\n",
               errno);
    }
    return 0;
}