#include <stdlib.h>
#include "dlmall.h"

#define ROUNDS 10000
#define BLOCKS 1000

int main() {
  int* blocks[BLOCKS];
  for (int i = 0; i < BLOCKS; i++) {
    blocks[i] = dalloc(16);
  }

  for (int i = 0; i < ROUNDS; i++) {
    for (int j = 0; j < BLOCKS; j++) {
      *blocks[j] = i+j;
    }
  }
}