#include "queue_a.h"

#ifndef __STREAMS_H__
#define __STREAMS_H__

/*
   One of these per stream.  Holds:  the mutex lock and notifier condition
   variables a buffer of tokens taken from a producer a structure with
   information regarding the processing of tokens an identity
*/
typedef struct stream_struct {
    pthread_mutex_t lock;
    pthread_cond_t notifier;
    queue buffer;               /* a buffer of values of any type      */
    void *args;                 /* arguments for the producing stream */
    int id;                     /* identity of this stream            */

    struct prod_list *prod_head;
    struct prod_list *prod_curr;
} stream_t;

struct prod_list {
    stream_t *prod;
    struct prod_list *next;
};


void *get(stream_t *stream);
void put(stream_t *stream, void *value);
void *successor(void *stream);
void *times(void *streams);
void *merge(void *streams);
void *consumer(void *streams);
void *consume_single(void *streams);
void init_stream(stream_t *stream, void *data);
void kill_stream(stream_t *stream);
void connect(stream_t *in, stream_t *out);

#endif
