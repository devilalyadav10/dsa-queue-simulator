#ifndef QUEUE_H
#define QUEUE_H

#define MAX_QUEUE_SIZE 200

typedef struct {
    char vehicle_id[9];
    char road;
    int lane;
} Vehicle;

typedef struct {
    Vehicle items[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int count;
    int priority;
} Queue;

void init_queue(Queue *q);
int is_empty(Queue *q);
int is_full(Queue *q);
void enqueue(Queue *q, Vehicle vehicle);
Vehicle dequeue(Queue *q);
int get_count(Queue *q);
void set_priority(Queue *q, int priority);
int get_priority(Queue *q);

#endif