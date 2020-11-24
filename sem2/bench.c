#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dlmall.h"

#define MIN 8
#define MAX 128
#define BLOCKS 256
#define ROUNDS 100
#define LOOP 100

void doAlloc() {
  struct head* blocks[BLOCKS];
  int fsize[ROUNDS];

  /* empty list */
  for (int i = 0; i < BLOCKS; i++) {
    blocks[i] = NULL;
  }

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
        //fprintf(stderr, "memory allocation failed.\n");
      }
    }
    int total = getAllocSize();
    int flistSize = getFreeLength();
    printf("%d %d\n", j, flistSize);
  }
  /* perform sanity check */
  //sanity();
}

int main() {
  doAlloc();



  // for (int j = 0; j < ROUNDS; j++) {
  //   for (int i = 0; i < LOOP; i++) {
  //     int index = rand() % BUFFER;
  //     if (buffer[index] != NULL) {
  //       free(buffer[index]);
  //       buffer[index] = NULL;
  //     }
  //     size_t size = (size_t)request();
  //     int* memory;
  //     memory = malloc(size);
  //     if (memory == NULL) {
  //       fprintf(stderr, "memmory allocation failed\n");
  //       return(1);
  //     }
  //     buffer[index] = memory;
  //     // writing to the memory so we know it exists
  //     *memory = 123;
  //   }
  //   current = sbrk(0);
  //   int allocated = (int)((current - init)/1024);
  //   printf("%d\n", j);
  //   printf("the current top of the heap is %p.\n", current);
  //   printf("  increased by %d Kbyte\n", allocated);
  // }
  return 0;
}