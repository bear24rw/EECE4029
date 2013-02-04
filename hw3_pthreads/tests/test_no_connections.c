#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "streams.h"

int main(void) {
    printf("01\t------------------------\n");
    printf("02\t1 successor, 3 consumers\n");
    printf("03\t------------------------\n");

    pthread_t s1;
    pthread_t c1;
    pthread_t c2;
    pthread_t c3;

    stream_t suc1;
    stream_t cons1;
    stream_t cons2;
    stream_t cons3;

    init_stream(&suc1, NULL);
    init_stream(&cons1, NULL);
    init_stream(&cons2, NULL);
    init_stream(&cons3, NULL);

    /*
    stream_connect(&cons1, &suc1);
    stream_connect(&cons2, &suc1);
    stream_connect(&cons3, &suc1);
    */

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&s1, &attr, successor, (void*)&suc1);
    pthread_create(&c1, &attr, consumer, (void*)&cons1);
    pthread_create(&c2, &attr, consumer, (void*)&cons2);
    pthread_create(&c3, &attr, consumer, (void*)&cons3);

    pthread_join(c1, NULL);
    pthread_join(c2, NULL);
    pthread_join(c3, NULL);

    pthread_cancel(s1);

    kill_stream(&suc1);

    pthread_exit(NULL);

    return 0;
}

