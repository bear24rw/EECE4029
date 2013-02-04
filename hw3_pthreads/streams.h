#include <semaphore.h>

#ifndef __STREAMS_H__
#define __STREAMS_H__

#define BUFFER_SIZE 5

/*
   One of these per stream.  Holds:  the mutex lock and notifier condition
   variables a buffer of tokens taken from a producer a structure with
   information regarding the processing of tokens an identity
*/
typedef struct stream_struct {
    int id;                     /* identity of this stream            */
    void *data;
    pthread_mutex_t lock;
    pthread_cond_t notifier;

    sem_t empty;
    void *buffer[BUFFER_SIZE];
    int buffer_read_count[BUFFER_SIZE];

    int put_idx;
    int num_consumers;

    struct prod_list *prod_head;
    struct prod_list *prod_curr;
} stream_t;

struct prod_list {
    int buffer_idx;
    stream_t *stream;
    struct prod_list *next;
    struct prod_list *prev;
};


void *get(struct prod_list *producer);
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
