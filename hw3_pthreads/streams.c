/* 07-prod-cons.c

   Producer-consumer co-routining.  There are five producers and four
   consumers.  Two successors put consecutive numbers into their respective
   streams.  Two times consumers multiply all consumed numbers by 5 and 7
   respectively and put the results into their streams.  A merge consumer
   merges the two stream created by the times producers.  A consumer prints
   tokens from the merge stream.  This  illustrates that producer-consumer
   relationships can be formed into complex networks.

   Each stream has a buffer of size 5 - producers can put up to 5 numbers
   in their stream before waiting.

   7,14,21,28...                1,2,3,4...
   /--- Times 7 <---- successor
   5,7,10,14...  /
   consumer <--- merge <----<
   \
   \--- Times 5 <---- successor
   5,10,15,20...              1,2,3,4...
   */
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "streams.h"
#include "queue_a.h"

pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

#define tprintf(...)    { \
    gettimeofday(&tv, NULL); \
    pthread_mutex_lock(&print_lock); \
    printf("%ld\t", tv.tv_sec*1000000+tv.tv_usec); \
    printf(__VA_ARGS__); \
    pthread_mutex_unlock(&print_lock); \
}

#define BUFFER_SIZE 5
int idcnt = 1;


/* return 'value' which should have a value from a call to put */
void *get(void *stream) {
    void *ret;  /* needed to take save a value from the critical section */
    queue *q = &((Stream*)stream)->buffer;
    pthread_mutex_t *lock = &((Stream*)stream)->lock;
    pthread_cond_t *notifier = &((Stream*)stream)->notifier;

    pthread_mutex_lock(lock);    /* lock other threads out of this section    */
    if (isEmpty(q))              /* if nothing in the buffer, wait and open   */
        pthread_cond_wait(notifier, lock);     /* the section to other threads */
    ret = dequeue(q);            /* take the next token from the buffer       */
    pthread_cond_signal(notifier);  /* if producer is waiting to add a token  */
    pthread_mutex_unlock(lock);     /* wake it up and unlock the section      */

    return ret;
}

/* 'value' is the value to move to the consumer */
void put(void *stream, void *value) {
    int j;
    queue *q = &((Stream*)stream)->buffer;
    pthread_mutex_t *lock = &((Stream*)stream)->lock;
    pthread_cond_t *notifier = &((Stream*)stream)->notifier;

    pthread_mutex_lock(lock);      /* lock the section */
    if (nelem(q) >= BUFFER_SIZE)   /* if buffer is full, cause the thread to */
        pthread_cond_wait(notifier, lock);  /* wait - unlock the section      */
    enqueue(q,value);              /* add the 'value' token to the buffer    */
    pthread_cond_signal(notifier); /* wake up a sleeping consumer, if any    */
    pthread_mutex_unlock(lock);    /* unlock the section */

    return;
}

/* Put 1,2,3,4,5... into the self stream */
void *successor (void *streams) {
    struct timeval tv;
    Stream *self = ((Args*)streams)->self;
    int id = ((Args*)streams)->self->id;
    int i, *value;

    for (i=1 ; ; i++) {
        /* sleep(1); */
        tprintf("Successor(%d): sending %d\n", id, i);
        value = (int*)malloc(sizeof(int));
        *value = i;
        put(self, (void*)value);
        tprintf("Successor(%d): sent %d, buf_sz=%d\n",
                id, i, nelem(&self->buffer));
    }
    pthread_exit(NULL);
}

/* multiply all tokens from the self stream by (int)self->args and insert
   the resulting tokens into the self stream */
void *times (void *streams) {
    struct timeval tv;
    Stream *self = ((Args*)streams)->self;
    Stream *prod = ((Args*)streams)->prod;
    int *value;
    void *in;

    tprintf("Times(%d) connected to Successor (%d)\n", self->id, prod->id);
    while (true) {
        in = get(prod);

        tprintf("\t\tTimes(%d): got %d from Successor %d\n",
                self->id, *(int*)in, prod->id);

        value = (int*)malloc(sizeof(int));
        *value = *(int*)in * (int)(self->args);
        free(in);
        put(self, (void*)value);

        tprintf("\t\tTimes(%d): sent %d buf_sz=%d\n",
                self->id, *value, nelem(&self->buffer));
    }
    pthread_exit(NULL);
}

/* merge two streams containing tokens in increasing order
ex: stream 1:  3,6,9,12,15,18...  stream 2: 5,10,15,20,25,30...
output stream: 3,5,6,9,10,12,15,15,18...
*/
void *merge (void *streams) {
    struct timeval tv;
    Stream *self = ((Args*)streams)->self;
    Stream *s1 = ((Args*)streams)->prod;
    Stream *s2 = (((Args*)streams)->prod)->next;
    void *a = get(s1);
    void *b = get(s2);

    while (true) {
        if (*(int*)a < *(int*)b) {
            put(self, a);
            a = get(s1);
            tprintf("\t\t\t\t\tMerge(%d): sent %d from Times %d buf_sz=%d\n",
                    self->id, *(int*)a, s1->id, nelem(&self->buffer));
        } else {
            put(self, b);
            b = get(s2);
            tprintf("\t\t\t\t\tMerge(%d): sent %d from Times %d buf_sz=%d\n",
                    self->id, *(int*)b, s2->id, nelem(&self->buffer));
        }
    }
    pthread_exit(NULL);
}

/* Final consumer in the network */
void *consumer (void *streams) {
    struct timeval tv;
    Stream *prod = ((Args*)streams)->prod;
    int i;
    void *value;

    for (i=0 ; i < 10 ; i++) {
        value = get(prod); 
        tprintf("\t\t\t\t\t\t\tConsumer: got %d\n", *(int*)value);
        free(value);
    }

    pthread_exit(NULL);
}

/* initialize streams - see also queue_a.h and queue_a.c */
void init_stream (Args *args, Stream *self, void *data) {
    if (self != NULL) {
        self->next = NULL;
        self->args = data;
        self->id = idcnt++;
        init_queue(&self->buffer);
        pthread_mutex_init(&self->lock, NULL);
        pthread_cond_init (&self->notifier, NULL);
    }
    args->self = self;
    args->prod = NULL;
}

/* free allocated space in the queue - see queue_a.h and queue_a.c */
void kill_stream(Stream *stream) { destroy_queue(&stream->buffer); }

/* puts an initialized stream object onto the end of a stream's input list */
void connect (Args *arg, Stream *s) {
    s->next = arg->prod;
    arg->prod = s;
}


