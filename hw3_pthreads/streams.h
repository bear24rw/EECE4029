#include "queue_a.h"

#ifndef __STREAMS_H__
#define __STREAMS_H__

/*
   One of these per stream.  Holds:  the mutex lock and notifier condition
   variables a buffer of tokens taken from a producer a structure with
   information regarding the processing of tokens an identity
*/
typedef struct stream_struct {
    struct stream_struct *next;
    pthread_mutex_t lock;
    pthread_cond_t notifier;
    queue buffer;               /* a buffer of values of any type      */
    void *args;                 /* arguments for the producing stream */
    int id;                     /* identity of this stream            */
} Stream;

/*
prod: linked list of streams that this producer consumes tokens from
self: the producer's output stream
*/
typedef struct {
    Stream *self, *prod;
} Args;

void *get(void *stream);
void put(void *stream, void *value);
void *successor(void *stream);
void *times(void *streams);
void *merge(void *streams);
void *consumer(void *streams);
void init_streams(Args *args, Stream *self, void *data);
void kill_stream(Stream *stream);
void connect(Args *arg, Stream *s);

#endif
