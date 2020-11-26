#include <stdlib.h>
#include <time.h>
#include "dlmall.h"

#define ROUNDS 10000
#define BLOCKS 1000

int main() {
  int* blocks[BLOCKS];
  clock_t rStart = clock();
  for (int i = 0; i < BLOCKS; i++) {
    blocks[i] = dalloc(16);
  }
  clock_t wStart = clock();
  for (int i = 0; i < ROUNDS; i++) {
    for (int j = 0; j < BLOCKS; j++) {
      *blocks[j] = i+j;
    }
  }
  time_t stop = clock();
  double rTime = (double)(wStart - rStart) / CLOCKS_PER_SEC;
  double wTime = (double)(stop - wStart) / CLOCKS_PER_SEC;
  double tTime = (double)(stop - rStart) / CLOCKS_PER_SEC;
  printf("Setup time: %f\nWrite time: %f\nTotal time: %f\n", rTime, wTime, tTime);
}