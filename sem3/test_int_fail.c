#include <stdio.h>
#include <time.h>
#include "green.h"

volatile int count = 0;

void* test(void* arg) {
  int id = *(int*)arg;
  int loop = 1000000;
  while (loop > 0) {
    loop--;
    count++;
  }
}


int main() {
  int a0 = 0;
  int a1 = 1;
  int a2 = 2;
  int a3 = 3;
  clock_t start = clock();

  green_t g0, g1, g2, g3;
  green_create(&g0, test, &a0);
  green_create(&g1, test, &a1);
  green_create(&g2, test, &a2);
  green_create(&g3, test, &a3);
  green_join(&g0, NULL);
  green_join(&g1, NULL);
  green_join(&g2, NULL);
  green_join(&g3, NULL);

  clock_t end = clock();
  int ms = ((double)(end-start)/CLOCKS_PER_SEC) * 1000;
  printf("count: %d, time: %dms\n", count, ms);
  return 0;
}