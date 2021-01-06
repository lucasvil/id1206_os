#include <stdio.h>
#include "green.h"

volatile int count = 0;
green_mutex_t mutex;

void* test(void* arg) {
  int id = *(int*)arg;
  int loop = 1000000;
  while (loop > 0) {
    green_mutex_lock(&mutex);
    printf("Thread %d: %d\n", id, loop);
    loop--;
    count++;
    green_mutex_unlock(&mutex);
  }
}

int main() {
  green_t g0, g1, g2, g3;
  int a0 = 0;
  int a1 = 1;
  int a2 = 2;
  int a3 = 3;

  green_mutex_init(&mutex);

  green_create(&g0, test, &a0);
  green_create(&g1, test, &a1);
  green_create(&g2, test, &a2);
  green_create(&g3, test, &a3);
  green_join(&g0, NULL);
  green_join(&g1, NULL);
  green_join(&g2, NULL);
  green_join(&g3, NULL);
  printf("count: %d\n", count);
  return 0;
}