/*
                       7,14,21,28...
                              /--- Times 7 <--\
               5,7,10,14...  /                 \       1,2,3,4...
   consumer <--- merge <----<                   ---- successor
                             \                 /
                              \--- Times 5 <--/
                       5,10,15,20...
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "streams.h"

int main () {

    int delay = 0;
    int times_5 = 5;
    int times_7 = 7;

    pthread_t t_s;  /* successor */
    pthread_t t_t5; /* times 7 */
    pthread_t t_t7; /* times 5 */
    pthread_t t_m;  /* merge */
    pthread_t t_c;  /* consumer */

    stream_t s_suc;
    stream_t s_times5;
    stream_t s_times7;
    stream_t s_merge;
    stream_t s_cons;

    init_stream(&s_suc    , (void*)&delay);
    init_stream(&s_times5 , (void*)&times_5);
    init_stream(&s_times7 , (void*)&times_7);
    init_stream(&s_merge  , NULL);
    init_stream(&s_cons   , (void*)&delay);

    stream_connect(&s_cons   , &s_merge);
    stream_connect(&s_merge  , &s_times5);
    stream_connect(&s_merge  , &s_times7);
    stream_connect(&s_times5 , &s_suc);
    stream_connect(&s_times7 , &s_suc);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&t_s  , &attr , successor , (void*)&s_suc);
    pthread_create(&t_t5 , &attr , times     , (void*)&s_times5);
    pthread_create(&t_t7 , &attr , times     , (void*)&s_times7);
    pthread_create(&t_m  , &attr , merge     , (void*)&s_merge);
    pthread_create(&t_c  , &attr , consumer  , (void*)&s_cons);

    pthread_join(t_c, NULL);

    pthread_cancel(t_m);
    pthread_cancel(t_t5);
    pthread_cancel(t_t7);
    pthread_cancel(t_s);

    return 0;
}
