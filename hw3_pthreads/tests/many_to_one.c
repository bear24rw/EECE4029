#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "streams.h"

int main(void) {
    printf("01\t------------------------\n");
    printf("02\t3 successors, 1 consumer\n");
    printf("03\t------------------------\n");

    int delay = 0;

    pthread_t s1;
    pthread_t s2;
    pthread_t s3;
    pthread_t c1;

    stream_t suc1;
    stream_t suc2;
    stream_t suc3;
    stream_t cons1;

    init_stream(&suc1, &delay);
    init_stream(&suc2, &delay);
    init_stream(&suc3, &delay);
    init_stream(&cons1, &delay);

    stream_connect(&cons1, &suc1);
    stream_connect(&cons1, &suc2);
    stream_connect(&cons1, &suc3);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&s1, &attr, successor, (void*)&suc1);
    pthread_create(&s2, &attr, successor, (void*)&suc2);
    pthread_create(&s3, &attr, successor, (void*)&suc3);
    pthread_create(&c1, &attr, consumer, (void*)&cons1);

    pthread_join(c1, NULL);

    pthread_cancel(s1);
    pthread_cancel(s2);
    pthread_cancel(s3);

    kill_stream(&suc1);
    kill_stream(&suc2);
    kill_stream(&suc3);

    return 0;
}

