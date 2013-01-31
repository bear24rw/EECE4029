#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "streams.h"

int main () {
    pthread_t s1, s2, t1, t2, m1, c1;
    Stream suc1, suc2, tms1, tms2, mrg;
    Args suc1_args, suc2_args, tms1_args, tms2_args, mrg_args, cons_args;
    pthread_attr_t attr;

    /* thread safe print mutex lock */
    //pthread_mutex_init(&print_lock, NULL);

    init_stream(&suc1_args, &suc1, NULL);   /* initialize a successor stream */

    init_stream(&suc2_args, &suc2, NULL);   /* initialize a successor stream */

    init_stream(&tms1_args, &tms1, (void*)7); /* initialize a times 7 stream */
    connect(&tms1_args, &suc1);               /* connect to a successor */

    init_stream(&tms2_args, &tms2, (void*)5); /* initialize a times 5 stream */
    connect(&tms2_args, &suc2);               /* connect to a successor */

    init_stream(&mrg_args, &mrg, NULL);     /* initialize a merge stream */
    connect(&mrg_args, &tms1);              /* connect to a times stream */
    connect(&mrg_args, &tms2);              /* connect to a 2nd times stream */

    init_stream(&cons_args, NULL, NULL);    /* initialize a consumer stream */
    connect(&cons_args, &mrg);              /* connect to a merge stream */

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&s1, &attr, successor, (void*)&suc1_args);
    pthread_create(&s2, &attr, successor, (void*)&suc2_args);
    pthread_create(&t1, &attr, times, (void*)&tms1_args);
    pthread_create(&t2, &attr, times, (void*)&tms2_args);
    pthread_create(&m1, &attr, merge, (void*)&mrg_args);
    pthread_create(&c1, &attr, consumer, (void*)&cons_args);

    pthread_join(c1, NULL);    /* cancel all running threads when the */
    /* is finished consuming */
    pthread_cancel(s1);
    pthread_cancel(s2);
    pthread_cancel(t1);
    pthread_cancel(t2);
    pthread_cancel(m1);

    kill_stream(&suc1);
    kill_stream(&suc2);
    kill_stream(&tms1);
    kill_stream(&tms2);
    kill_stream(&mrg);

    pthread_exit(NULL);
}
