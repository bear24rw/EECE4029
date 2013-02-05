#include <semaphore.h>

#ifndef __STREAMS_H__
#define __STREAMS_H__

#define BUFFER_SIZE 5

/*
   One of these per stream.  Holds:  the mutex lock and notifier condition
   variables a buffer of tokens taken from a producer a structure with
   information regarding the processing of tokens an identity
*/

typedef struct stream_t stream_t;
typedef struct producer_t producer_t;

struct stream_t {
    int id;                                 /* unique stream id */
    void *data;                             /* delay / multiplier / etc.. */
    pthread_mutex_t lock;                   /* mutex lock for buffer and notifier */
    pthread_cond_t notifier;                /* notifier to sleep and be woken up when even occur */

    sem_t empty;                            /* keeps track of how many empty sports there are in the buffer */
    void *buffer[BUFFER_SIZE];              /* void pointer buffer to store any type of data */
    int buffer_read_count[BUFFER_SIZE];     /* count of how many consumers have read from each index */

    int put_idx;                            /* producers index into the buffer */
    int num_consumers;                      /* how many consumers are connected to this producer */

    producer_t *prod_head;                  /* head of the producer linked list */
    producer_t *prod_curr;                  /* used for building the linked list */
};

/*
   A linked list of all the producers a consumer consumes from.
   'buffer_idx' is the read index into the producers buffer.
*/
struct producer_t {
    int buffer_idx;         /* read idex into the producers buffer */
    stream_t *stream;       /* the actual producer stream */
    producer_t *next;       /* the next producer in our list */
    producer_t *prev;       /* the previous producer in our list */
};


void *get(producer_t *producer);
void put(stream_t *stream, void *value);
void *successor(void *stream);
void *times(void *stream);
void *merge(void *stream);
void *consumer(void *streams);
void *consume_single(stream_t *stream);
void init_stream(stream_t *stream, void *data);
void kill_stream(stream_t *stream);
void stream_connect(stream_t *in, stream_t *out);
void stream_disconnect(stream_t *in, stream_t *out);

#endif
