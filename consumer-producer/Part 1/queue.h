/*
    This code was taken from: 
    http://www.thelearningpoint.net/computer-science/data-structures-queues--with-c-program-source-code 
    and modified by our own purposes. We take no credit for writing this code, just for modifying it to
    fit our needs.
*/

#ifndef POINT_H_
#define POINT_H_

typedef struct Queue
{
        int capacity;
        //int size;
        int front;
        int rear;
        int *elements;
}Queue;

Queue * createQueue(int maxElements);
void queue_destroy(Queue *Q);
int Dequeue(Queue *Q);
void Enqueue(Queue *Q,int element);

#endif /* POINT_H_ */
