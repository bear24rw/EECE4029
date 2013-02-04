#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "streams.h"

int main(void) {
    printf("01\t------------------------\n");
    printf("02\t1 successor, 2 consumers\n");
    printf("03\tConsumer 2 slower\n");
    printf("03\t------------------------\n");

    int delay_zero = 0;
    int delay_1 = 1;
    int delay_2 = 10;

    pthread_t s1;
    pthread_t c1;
    pthread_t c2;

    stream_t suc1;
    stream_t cons1;
    stream_t cons2;

    init_stream(&suc1, &delay_zero);
    init_stream(&cons1, &delay_1);
    init_stream(&cons2, &delay_2);

    stream_connect(&cons1, &suc1);
    stream_connect(&cons2, &suc1);

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

    return 0;
}

