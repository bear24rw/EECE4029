#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>
#include <semaphore.h>
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

void *get(producer_t *producer)
{
    //struct timeval tv;
    void *ret;  /* needed to take save a value from the critical section */

    int *buffer_idx          = &producer->buffer_idx;
    pthread_mutex_t *lock    = &producer->stream->lock;
    pthread_cond_t *notifier = &producer->stream->notifier;
    sem_t *empty             = &producer->stream->empty;

    /* make sure no other getters come in here */
    pthread_mutex_lock(lock);

    /* if we are ahead where the producer is writing, wait */
    while ((*buffer_idx+1)%BUFFER_SIZE == (producer->stream->put_idx)%BUFFER_SIZE) {
        //tprintf("\tGetter caught up to putter, waiting at buff idx %d\n", *buffer_idx);
        pthread_cond_wait(notifier, lock);
    }

    /* if there isn't anything new here, wait */
    while (producer->stream->buffer_read_count[*buffer_idx] >= producer->stream->num_consumers) {
        //tprintf("\tGetter found nothing new at idx %d, waiting\n", *buffer_idx)
        pthread_cond_wait(notifier, lock);
    }

/*
    tprintf("\t\tGetting from buf idx %d (read count: %d) current put idx %d\n",
            *buffer_idx,
            producer->stream->buffer_read_count[*buffer_idx],
            producer->stream->put_idx);
*/

    /* get the value out of the producer streams buffer */
    ret = producer->stream->buffer[*buffer_idx];

    /* decrease the read count since we just got a value */
    producer->stream->buffer_read_count[*buffer_idx]++;

    /* if we are last getter, the spot is now empty */
    if(producer->stream->buffer_read_count[*buffer_idx] == producer->stream->num_consumers)
        sem_post(empty);

    /* go to the next buffer location for next time*/
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
    sem_t *empty             = &stream->empty;

    /* wait until there are empty slots in the buffer */
    sem_wait(empty);

    pthread_mutex_lock(lock);

    /* wait if all consumers haven't seen this value */
    while (stream->buffer_read_count[stream->put_idx] < stream->num_consumers) {
        //tprintf("Put read count at idx %d is %d, waiting\n", stream->put_idx, stream->buffer_read_count[stream->put_idx]);
        //tprintf("Num consumers: %d\n", stream->num_consumers);
        pthread_cond_wait(notifier, lock);
    }

    /* put the new value in the buffer */
    stream->buffer[stream->put_idx] = value;

    /* reset the read cound since this is a fresh value */
    stream->buffer_read_count[stream->put_idx] = 0;
    //tprintf("Putting '%d' at idx %d\n", *(int*)value, stream->put_idx);

    /* go next buffer position for next time */
    stream->put_idx = (stream->put_idx + 1) % BUFFER_SIZE;

    /* notify the consumer that we've updated */
    pthread_cond_signal(notifier);

    pthread_mutex_unlock(lock);

    return;
}

/* Put 1,2,3,4,5... into a stream */
void *successor (void *stream) {
    struct timeval tv;
    stream_t *self = (stream_t*)stream;
    int delay = *(int*)self->data;
    int id = self->id;
    int i, *value;

    for (i=1 ; ; i++) {
        sleep(delay);
        //tprintf("Successor(%d): sending %d\n", id, i);
        value = (int*)malloc(sizeof(int));
        *value = i;
        put(self, (void*)value);
        tprintf("Successor(%d): sent %d\n", id, i);
    }
    pthread_exit(NULL);
}

/* multiply all tokens from the self stream by (int)self->args and insert
   the resulting tokens into the self stream */
void *times (void *stream) {
    struct timeval tv;
    stream_t *self = (stream_t *)stream;
    producer_t *p = self->prod_head;
    int multiplier = *(int*)self->data;
    int in;
    int *out;

    tprintf("Times(%d) connected to Successor (%d)\n", self->id, p->stream->id);

    while (true) {
        p = self->prod_head;
        while (p != NULL)
        {
            in = *(int*)get(p);

            tprintf("\t\tTimes(%d): got %d from Successor %d\n", self->id, in, p->stream->id); 

            out = (int*)malloc(sizeof(int));
            *out = in * multiplier;
            put(self, (void*)out);

            tprintf("\t\tTimes(%d): sent %d\n", self->id, *out);
        }
    }
    pthread_exit(NULL);
}


/* merge two streams containing tokens in increasing order
ex: stream 1:  3,6,9,12,15,18...  stream 2: 5,10,15,20,25,30...
output stream: 3,5,6,9,10,12,15,15,18...
*/

void *merge (void *stream) {
    struct timeval tv;

    stream_t *self = (stream_t *)stream;

    producer_t *p1 = self->prod_head;
    producer_t *p2 = self->prod_head->next;

    void *a = get(p1);
    void *b = get(p2);

    while (true) {
        if (*(int*)a < *(int*)b) {
            put(self, a);
            a = get(p1);
            tprintf("\t\t\t\t\tMerge(%d): sent %d from Times %d\n",
                    self->id, *(int*)a, p1->stream->id);
        } else {
            put(self, b);
            b = get(p2);
            tprintf("\t\t\t\t\tMerge(%d): sent %d from Times %d\n",
                    self->id, *(int*)b, p1->stream->id);
        }
    }
    pthread_exit(NULL);
}


void *consumer(void *stream)
{
    struct timeval tv;
    stream_t *self = (stream_t *)stream;
    producer_t *p = self->prod_head;
    int delay = *(int*)self->data;
    int i;
    void *value;

    for (i=0 ; i < 10 ; i++)
    {
        sleep(delay);
        p = self->prod_head;
        while (p != NULL)
        {
            value = get(p);
            tprintf("\t\t\t\t\t\t\tConsumer %d: got %d\n", self->id, *(int*)value);
            p = p->next;
        }
    }

    pthread_exit(NULL);
}

void *consume_single(stream_t *stream) {
    producer_t *p = stream->prod_head;
    return get(p);
}


/* initialize streams - see also queue_a.h and queue_a.c */
void init_stream(stream_t *stream, void *data) {
    stream->id = idcnt++;
    stream->data = data;
    pthread_mutex_init(&stream->lock, NULL);
    pthread_cond_init (&stream->notifier, NULL);
    stream->prod_head = NULL;
    stream->prod_curr = NULL;
    stream->put_idx = 0;
    stream->num_consumers = 0;
    int i;
    for (i=0; i<BUFFER_SIZE; i++)
        stream->buffer_read_count[i] = 9999;
    sem_init(&stream->empty, 0, BUFFER_SIZE);
}

/* free allocated space in the queue - see queue_a.h and queue_a.c */
void kill_stream(stream_t *stream) { }

void stream_connect (stream_t *in, stream_t *out) {

    /* add the producer to the consumers list of producers */
    producer_t *p = (producer_t*)malloc(sizeof(producer_t));

    p->buffer_idx = out->put_idx;
    p->stream = out;
    p->next = NULL;
    p->prev = NULL;

    if (in->prod_head == NULL) {
        in->prod_head = p;
        in->prod_curr = p;
    } else {
        p->prev = in->prod_curr;
        in->prod_curr->next = p;
        in->prod_curr = p;
    }

    out->num_consumers++;
    pthread_cond_signal(&out->notifier);
    pthread_cond_signal(&in->notifier);
}

void stream_disconnect(stream_t *in, stream_t *out) {

    producer_t *p = in->prod_head;

    while (p != NULL)
    {
        if (p->stream == out) {
            if (p->prev != NULL) {
                p->prev->next = p->next;
                if (p->next != NULL) {
                    p->next->prev = p->prev;
                }
            }
            out->num_consumers--;
            int i;
            for (i = 0; i < BUFFER_SIZE; i++)
                if (p->stream->buffer_read_count[i] > 0)
                    p->stream->buffer_read_count[i]--;

            free(p);
            break;
        }
        p = p->next;
    }
}

