#include <stdlib.h>
#include "dlmall.h"

int main(void) {
  init();
  struct head* block1 = dalloc(100); // should alloc 136 = 100 + 24 + 8
  struct head* block2 = dalloc(100);
  struct head* block3 = dalloc(100);
  dfree(block3);
  struct head* block4 = dalloc(200);
  dfree(block2);

  // print entire arena
  printArena();
}
