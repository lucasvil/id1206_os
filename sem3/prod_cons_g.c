#include <stdio.h>
#include <stdlib.h>
#include "green.h"

#define BSIZE 4

int* buffer[BSIZE];
int count = 0;
green_mutex_t mutex;
green_cond_t cond;

void produce(int i) {
  int r = rand();
  int* prod = &r;
  printf("PRODUCING! count=%d i=%d\n", count, i);
  buffer[i % BSIZE] = prod;
}

void consume(int i) {
  printf("CONSUMING! count=%d i=%d\n", count, i);
  buffer[i % BSIZE] = NULL;
}

void* producer() {
  for (int i = 0; i < 1000; i++) {
    green_mutex_lock(&mutex);
    if (count == BSIZE) {
      green_cond_wait(&cond, &mutex);
    }
    produce(i);
    count++;
    if (count == 1) {
      green_cond_signal(&cond);
    }
    green_mutex_unlock(&mutex);
  }
}

void* consumer() {
  for (int i = 0; i < 1000; i++) {
    green_mutex_lock(&mutex);
    if (count == 0) {
      green_cond_wait(&cond, &mutex);
    }
    count--;
    if (count == BSIZE - 1) {
      green_cond_signal(&cond);
    }
    consume(i);
    green_mutex_unlock(&mutex);
  }
}

int main()
{
  int a0 = 0;
  int a1 = 1;
  green_t p, c;
  printf("started\n");
  green_cond_init(&cond);
  green_mutex_init(&mutex);
  green_create(&p, producer, &a0);
  green_create(&c, consumer, &a1);
  green_join(&p, NULL);
  green_join(&c, NULL);
  printf("done\n");
}