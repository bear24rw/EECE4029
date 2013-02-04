#include "queue_a.h"

#ifndef __STREAMS_H__
#define __STREAMS_H__

#define BUFFER_SIZE 5

/*
   One of these per stream.  Holds:  the mutex lock and notifier condition
   variables a buffer of tokens taken from a producer a structure with
   information regarding the processing of tokens an identity
*/
typedef struct stream_struct {
    pthread_mutex_t lock;
    pthread_cond_t notifier;
    void *buffer[BUFFER_SIZE];
    int id;                     /* identity of this stream            */

    int put_idx;

    struct prod_list *prod_head;
    struct prod_list *prod_curr;

    struct cons_list *cons_head;
    struct cons_list *cons_curr;

} stream_t;

struct prod_list {
    int buffer_idx;
    stream_t *stream;
    struct prod_list *next;
};

struct cons_list {
    struct prod_list *prod;
    struct cons_list *next;
};


void *get(struct prod_list *producer);
void put(stream_t *stream, void *value);
void *successor(void *stream);
void *times(void *streams);
void *merge(void *streams);
void *consumer(void *streams);
void *consume_single(stream_t *stream);
void init_stream(stream_t *stream, void *data);
void kill_stream(stream_t *stream);
void stream_connect(stream_t *in, stream_t *out);

#endif
