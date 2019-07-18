typedef struct node Node;
typedef struct queue Queue;

#include <stdbool.h>
Queue *newQueue(void);
char dequeue(Queue *queue);
int enqueue(Queue *queue, char data);
int getSize(Queue *queue);
bool getIsFullFlag(Queue*);
bool getIsEmptyFlag(Queue *queue);
int printQueue(Queue *queue);