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
#include <sys/time.h>
#include "streams.h"

pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

#define tprintf(...)    { \
    gettimeofday(&tv, NULL); \
    pthread_mutex_lock(&print_lock); \
    printf("%ld\t", tv.tv_sec*1000000+tv.tv_usec); \
    printf(__VA_ARGS__); \
    pthread_mutex_unlock(&print_lock); \
}

int idcnt = 1;

void *get(struct prod_list *producer)
{
    //struct timeval tv;
    void *ret;  /* needed to take save a value from the critical section */

    int *buffer_idx          = &producer->buffer_idx;
    pthread_mutex_t *lock    = &producer->stream->lock;
    pthread_cond_t *notifier = &producer->stream->notifier;

    /* make sure no other getters come in here */
    pthread_mutex_lock(lock);

    /* if we caught up to where the producer is writing, wait */
    if (*buffer_idx == producer->stream->put_idx) {
        //tprintf("\t\tWaiting at buff idx %d\n", *buffer_idx);
        pthread_cond_wait(notifier, lock);
    }

    /* get the value out of the producer streams buffer */
    ret = producer->stream->buffer[*buffer_idx];
    //tprintf("\t\tGetting from buf idx %d, put idx %d\n", *buffer_idx, producer->stream->put_idx);

    /* go to the next buffer location */
    *buffer_idx = (*buffer_idx + 1) % BUFFER_SIZE;

    /* notify producer that we've moved on */
    pthread_cond_signal(notifier);

    /* allow other getters to come in */
    pthread_mutex_unlock(lock);

    return ret;
}

void put(stream_t *stream, void *value)
{
    //struct timeval tv;
    pthread_mutex_t *lock    = &stream->lock;
    pthread_cond_t *notifier = &stream->notifier;

    pthread_mutex_lock(lock);              /* lock the section */

    /*
     * loop through all getters and see if we're now at the same position as
     * one of them. if we are  we need to wait until they notify us that they
     * moved.
     */

    int wait;
    struct cons_list *c;

    do {
        /* assume we don't have to wait */
        wait = 0;
        c = stream->cons_head;
        while (c != NULL)
        {
            if (c->prod->buffer_idx == (stream->put_idx + 1) % BUFFER_SIZE) {
                //tprintf("Putter waiting at idx %d\n", stream->put_idx);
                wait = 1;
                break;
            }
            c = c->next;
        }
        /* wait until a getter notifies us */
        if (wait) pthread_cond_wait(notifier, lock);

    /* check again since things might have changed while we were waiting */
    } while (wait == 1);

    /* we are clean to go to next buffer position */
    stream->put_idx = (stream->put_idx + 1) % BUFFER_SIZE;

    /* put the new value in the buffer */
    stream->buffer[stream->put_idx] = value;
    //tprintf("Putting <%d> at idx %d\n", *(int*)value, stream->put_idx);


    pthread_cond_signal(notifier);         /* wake up a sleeping consumer, if any    */
    pthread_mutex_unlock(lock);            /* unlock the section */

    return;
}

/* Put 1,2,3,4,5... into a stream */
void *successor (void *stream) {
    struct timeval tv;
    stream_t *self = (stream_t*)stream;
    int id = self->id;
    int i, *value;

    for (i=1 ; ; i++) {
        //sleep(1);
        tprintf("Successor(%d): sending %d\n", id, i);
        value = (int*)malloc(sizeof(int));
        *value = i;
        put(self, (void*)value);
        //tprintf("Successor(%d): sent %d, buf_sz=%d\n",
                //id, i, nelem(&self->buffer));
    }
    pthread_exit(NULL);
}

/* multiply all tokens from the self stream by (int)self->args and insert
   the resulting tokens into the self stream */
/*
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
        *value = *(int*)in * *(int*)(self->args);
        free(in);
        put(self, (void*)value);

        tprintf("\t\tTimes(%d): sent %d buf_sz=%d\n",
                self->id, *value, nelem(&self->buffer));
    }
    pthread_exit(NULL);
}
*/

/* merge two streams containing tokens in increasing order
ex: stream 1:  3,6,9,12,15,18...  stream 2: 5,10,15,20,25,30...
output stream: 3,5,6,9,10,12,15,15,18...
*/
/*
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
*/

void *consumer(void *stream)
{
    struct timeval tv;
    stream_t *self = (stream_t *)stream;
    struct prod_list *p = self->prod_head;
    int i;
    void *value;

    for (i=0 ; i < 10 ; i++)
    {
        sleep(1);
        p = self->prod_head;
        while (p != NULL)
        {
            value = get(p);
            tprintf("\t\t\t\t\t\t\tConsumer %d: got %d\n", self->id, *(int*)value);
            //free(value);

            p = p->next;
        }
    }

    pthread_exit(NULL);
}

void *consume_single(stream_t *stream) {
    struct prod_list *p = stream->prod_head;
    return get(p);
}


/* initialize streams - see also queue_a.h and queue_a.c */
void init_stream(stream_t *stream, void *data) {
    stream->id = idcnt++;
    pthread_mutex_init(&stream->lock, NULL);
    pthread_cond_init (&stream->notifier, NULL);
    stream->prod_head = NULL;
    stream->prod_curr = NULL;
    stream->cons_head = NULL;
    stream->cons_curr = NULL;
    stream->put_idx = 0;
}

/* free allocated space in the queue - see queue_a.h and queue_a.c */
void kill_stream(stream_t *stream) { }

void stream_connect (stream_t *in, stream_t *out) {

    /* add the producer to the consumers list of producers */
    struct prod_list *p = (struct prod_list*)malloc(sizeof(struct prod_list));

    p->buffer_idx = 0;
    p->stream = out;
    p->next = NULL;

    if (in->prod_head == NULL) {
        in->prod_head = p;
        in->prod_curr = p;
    } else {
        in->prod_curr->next = p;
        in->prod_curr = p;
    }

    /* add the consumer to the producer list of consumers */
    struct cons_list *c = (struct cons_list*)malloc(sizeof(struct cons_list));

    c->prod = p;
    c->next = NULL;

    if (out->cons_head == NULL) {
        out->cons_head = c;
        out->cons_curr = c;
    } else {
        out->cons_curr->next = c;
        out->cons_curr = c;
    }
}



