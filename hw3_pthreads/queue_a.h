#ifndef _QUEUE_A
#define _QUEUE_A

typedef struct {
    void **data;
    int tail, head, size, nelem;
} queue;

void init_queue(queue*);
void enqueue(queue*, void*);
void *dequeue(queue*);
bool isEmpty(queue*);
int nelem(queue*);
void print (queue*);
void destroy_queue(queue*);

#endif
