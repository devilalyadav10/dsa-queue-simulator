#include "queue.h"
#include <string.h>

void init_queue(Queue *q)
{
    q->front = 0;
    q->rear = -1;
    q->count = 0;
    q->priority = 0;
}

int is_empty(Queue *q)
{
    return q->count == 0;
}

int is_full(Queue *q)
{
    return q->count == MAX_QUEUE_SIZE;
}

void enqueue(Queue *q, Vehicle vehicle)
{
    if (!is_full(q))
    {
        q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
        memcpy(&q->items[q->rear], &vehicle, sizeof(Vehicle));
        q->count++;
    }
}

Vehicle dequeue(Queue *q)
{
    Vehicle empty = {"", ' ', 0};
    if (!is_empty(q))
    {
        Vehicle item = q->items[q->front];
        q->front = (q->front + 1) % MAX_QUEUE_SIZE;
        q->count--;
        return item;
    }
    return empty;
}

int get_count(Queue *q)
{
    return q->count;
}

void set_priority(Queue *q, int priority)
{
    q->priority = priority;
}

int get_priority(Queue *q)
{
    return q->priority;
}