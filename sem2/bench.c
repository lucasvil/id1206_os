#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dlmall.h"

#define MIN 8
#define MAX 128
#define BLOCKS 128
#define ROUNDS 100
#define LOOP 100

int main() {
  struct head* blocks[BLOCKS];
  int fsize[ROUNDS];

  /* empty list */
  for (int i = 0; i < BLOCKS; i++) {
    blocks[i] = NULL;
  }
  int fails = 0;

  /* 100 loops for 10 rounds */
  for (int j = 0; j < ROUNDS; j++) {
    for (int i = 0; i < LOOP; i++) {
      int index = rand() % BLOCKS;
      if (blocks[i] != NULL) {
        dfree(blocks[i]);
        blocks[i] = NULL;
      }
      size_t request = (rand() % (MAX - MIN)) + MIN;
      if ((blocks[i] = dalloc(request)) == NULL) {
        fails++;
      }
    }
    int total = getAllocSize();
    int flistSize = getFreeLength();
    printf("%d %d\n", j, fails);
  }
  //printf("%d\n", fails);
  return 0;
}