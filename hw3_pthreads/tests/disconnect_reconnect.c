#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "streams.h"

void print_buffers(stream_t *stream)
{
    producer_t *p = stream->prod_head;

    int i;
    printf("VALUE | ");
    for (i = 0; i < BUFFER_SIZE; i++)
        printf("%02d | ", *(int*)p->stream->buffer[i]);
    printf("\n");

    printf("COUNT | ");
    for (i = 0; i < BUFFER_SIZE; i++)
        printf("%02d | ", p->stream->buffer_read_count[i]);
    printf("\n");

    printf("P IDX | ");
    for (i = 0; i < BUFFER_SIZE; i++)
        if (i == p->stream->put_idx)
            printf("^^   ");
        else
            printf("     ");
    printf("\n");

    printf("G IDX | ");
    for (i = 0; i < BUFFER_SIZE; i++)
        if (i == p->buffer_idx)
            printf("^^   ");
        else
            printf("     ");
    printf("\n");

}

void *suc(void *stream) {
    stream_t *self = (stream_t*)stream;
    int delay = *(int*)self->data;
    int i, *value;

    for (i=1 ; ; i++) {
        sleep(delay);
        value = (int*)malloc(sizeof(int));
        *value = i;
        put(self, (void*)value);
    }
    pthread_exit(NULL);
}

int main(void) {
    printf("01\t--------------------------------------------\n");
    printf("02\t1 successor, 2 non threaded consumers\n");
    printf("03\tConsumer 2 disconnects, consumer 1 continues\n");
    printf("04\tgetting token, consumer 2 reconnects\n");
    printf("03\t--------------------------------------------\n");

    int con_delay = 0;
    int suc_delay = 0;

    pthread_t s1;

    stream_t suc1;
    stream_t cons1;
    stream_t cons2;

    init_stream(&suc1, &suc_delay);
    init_stream(&cons1, NULL);
    init_stream(&cons2, NULL);

    stream_connect(&cons1, &suc1);
    stream_connect(&cons2, &suc1);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&s1, &attr, suc, (void*)&suc1);

    printf("Both consumers connected\n");
    sleep(con_delay); printf("Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("Consumer 2 got: %d\n", *(int*)consume_single((void*)&cons2)); //print_buffers(&cons2);
    sleep(con_delay); printf("Consumer 2 got: %d\n", *(int*)consume_single((void*)&cons2)); //print_buffers(&cons2);
    sleep(con_delay); printf("Consumer 2 got: %d\n", *(int*)consume_single((void*)&cons2)); //print_buffers(&cons2);
    printf("Disconnecting consumer 2\n");
    sleep(con_delay); stream_disconnect(&cons2, &suc1);
    sleep(con_delay); printf("  Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("  Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("  Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("  Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("  Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("  Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("  Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("  Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    printf("Reconnecting consumer 2\n");
    sleep(con_delay); stream_connect(&cons2, &suc1);
    sleep(con_delay); printf("Consumer 2 got: %d\n", *(int*)consume_single((void*)&cons2)); //print_buffers(&cons2);
    sleep(con_delay); printf("Consumer 2 got: %d\n", *(int*)consume_single((void*)&cons2)); //print_buffers(&cons2);
    sleep(con_delay); printf("Consumer 2 got: %d\n", *(int*)consume_single((void*)&cons2)); //print_buffers(&cons2);
    sleep(con_delay); printf("Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    printf("Disconnecting consumer 1\n");
    sleep(con_delay); stream_disconnect(&cons1, &suc1);
    sleep(con_delay); printf("Consumer 2 got: %d\n", *(int*)consume_single((void*)&cons2)); //print_buffers(&cons2);
    sleep(con_delay); printf("Consumer 2 got: %d\n", *(int*)consume_single((void*)&cons2)); //print_buffers(&cons2);
    sleep(con_delay); printf("Consumer 2 got: %d\n", *(int*)consume_single((void*)&cons2)); //print_buffers(&cons2);
    printf("Reconnecting consumer 1\n");
    sleep(con_delay); stream_connect(&cons1, &suc1);
    sleep(con_delay); printf("Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);
    sleep(con_delay); printf("Consumer 1 got: %d\n", *(int*)consume_single((void*)&cons1)); //print_buffers(&cons1);

    sleep(con_delay); printf("done\n");

    pthread_cancel(s1);

    kill_stream(&suc1);

    return 0;
}

