#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "streams.h"

int main(void) {
    printf("01\t------------------------------------\n");
    printf("02\t1 successor, 1 non threaded consumer\n");
    printf("03\t------------------------------------\n");

    int suc_delay = 0;
    int cons_delay = 0;

    pthread_t s1;

    stream_t suc1;
    stream_t cons1;

    init_stream(&suc1, &suc_delay);
    init_stream(&cons1, &cons_delay);

    stream_connect(&cons1, &suc1);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&s1, &attr, successor, (void*)&suc1);

    printf("Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1));
    printf("Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1));
    printf("Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1));

    pthread_cancel(s1);

    kill_stream(&suc1);

    return 0;
}

