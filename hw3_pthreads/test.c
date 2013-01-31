#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "streams.h"

void main(void) {
    pthread_t s1;
    pthread_t c1;
    pthread_t c2;

    stream_t suc1;
    stream_t cons1;
    stream_t cons2;

    init_stream(&suc1, NULL);
    init_stream(&cons1, NULL);
    init_stream(&cons2, NULL);

    connect(&cons1, &suc1);
    connect(&cons2, &suc1);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&s1, &attr, successor, (void*)&suc1);
    pthread_create(&c1, &attr, consumer, (void*)&cons1);
    pthread_create(&c2, &attr, consumer, (void*)&cons2);

    pthread_join(c1, NULL);
    pthread_join(c2, NULL);

    pthread_cancel(s1);

    kill_stream(&suc1);

    pthread_exit(NULL);
}

