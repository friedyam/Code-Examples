/*
    This code was taken from: 
    http://www.thelearningpoint.net/computer-science/data-structures-queues--with-c-program-source-code 
    and modified by our own purposes. We take no credit for writing this code, just for modifying it to
    fit our needs.
*/

#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

/* createQueue function takes argument the maximum number of elements the Queue can hold, creates
   a Queue according to it and returns a pointer to the Queue. */
Queue * createQueue(int maxElements) {
        /* Create a Queue */
        Queue *Q;
        Q = (Queue *)malloc(sizeof(Queue));
        /* Initialise its properties */
        Q->elements = (int *)malloc(sizeof(int)*maxElements);
        Q->capacity = maxElements;
        Q->front = 0;
        Q->rear = -1;
        /* Return the pointer */
        return Q;
}

void queue_destroy(Queue *Q){
    free(Q->elements);
    free(Q);
    return;
}

int Dequeue(Queue *Q) {
    int value = Q->elements[Q->front];
    Q->front++;
    /* As we fill elements in circular fashion */
    if(Q->front == Q->capacity)
    {
        Q->front=0;
    }
    return value;
}

void Enqueue(Queue *Q,int element) {
    Q->rear = Q->rear + 1;
    /* As we fill the queue in circular fashion */
    if(Q->rear == Q->capacity)
    {
        Q->rear = 0;
    }
    /* Insert the element in its rear side */ 
    Q->elements[Q->rear] = element;
        
    return;
}