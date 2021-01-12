#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "green.h"


void* test(void* arg) {
  int i = *(int*)arg;
  int loop = 10000;
  while (loop > 0) {
    printf("thread: %d %d\n", i, loop);
    loop--;
    green_yield();
  }
}

void* test_p(void* arg) {
  int i = *(int*)arg;
  int loop = 10000;
  while (loop > 0) {
    printf("thread: %d %d\n", i, loop);
    loop--;
  }
}

int main() {
  int a0 = 0;
  int a1 = 1;
  int round = 0;
  int ms = 0;
  while (round < 100) {
    clock_t start = clock();
    /* green thread */
    green_t g0, g1;
    green_create(&g0, test, &a0);
    green_create(&g1, test, &a1);
    green_join(&g0, NULL);
    green_join(&g1, NULL);

    /* pthread */
    // pthread_t p0, p1;
    // pthread_create(&p0, NULL, test_p, &a0);
    // pthread_create(&p1, NULL, test_p, &a1);
    // pthread_join(p0, NULL);
    // pthread_join(p1, NULL);

    clock_t end = clock();
    ms += ((double)(end-start)/CLOCKS_PER_SEC) * 1000;
    printf("%d %d\n", round, ms);
    round++;
  }
  return 0;
}