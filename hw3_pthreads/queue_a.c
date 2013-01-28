#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "queue_a.h"

void init_queue(queue *q) {
   q->size = 10;
   q->data = (void**)malloc(q->size*sizeof(void*));
   q->head = q->tail = q->nelem = 0;
}

void enqueue(queue *q, void *item) {
   if (item == NULL) return;
   if ((q->head == 0 && q->tail == q->size-1) || q->head == q->tail+1) {
      printf("No space left - cannot add - 1\n");
      return;
   }
   q->data[q->tail] = item;
   q->nelem = q->nelem+1;
   /* Special case, move head to empty element */
   if (q->head == q->tail) q->head = q->size-1;
   if (q->tail == q->size-1) q->tail = 0; else q->tail++;
}

void *dequeue(queue *q) {
   void *object = NULL;

   if (q->tail == q->head) return NULL; /* empty list */
   if (q->tail > q->head) {
      q->head = q->head+1;
      object = q->data[q->head];
      /* an empty queue, initialize */
      if (q->head+1 == q->tail) q->head = q->tail = 0;
      q->nelem = q->nelem-1;
      return object;
   } else {
      q->head = (q->head+1) % q->size;
      object = q->data[q->head];
      /* an empty queue, initialize */
      if (((q->head+1) % q->size) == q->tail) q->head = q->tail = 0;
      q->nelem = q->nelem-1;
      return object;
   }
}

bool isEmpty(queue *q) {  return q->tail == q->head;  }

int nelem(queue *q) { return q->nelem; }

void print (queue *q) {
   int i;

   printf("q->size=%d q->head=%d q->tail=%d\n",q->size,q->head,q->tail);
   fflush(stdout);
   if (isEmpty(q)) {
      printf("(empty)\n");
   } else {
      printf("[");fflush(stdout);
      for (i=q->head+1 ; i < q->size && i != q->tail ; i++) {
	 printf("%d ",q->data[i]); fflush(stdout);
      }
      if (i == q->size) {
	 for (i=0 ; i < q->tail ; i++) {
	    printf("%d ",q->data[i]); fflush(stdout);
	 }
      }
      printf("]\n");fflush(stdout);
   }
}

void destroy_queue (queue *q) { free(q->data); }
