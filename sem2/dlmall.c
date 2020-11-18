#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <errno.h>

#define TRUE 1
#define FALSE 0

#define HEAD (sizeof(struct head))
#define MIN(size) (((size)>(8))?(size):(8))
#define LIMIT(size) (MIN(0) + HEAD + size)
#define MAGIC(memory)((struct head*)memory - 1)
#define HIDE(block) (void*)((struct head*)block + 1)
#define ALIGN 8
#define ARENA (64*1024)

struct head {
  uint16_t bfree;   // 2 bytes, the status of block before
  uint16_t bsize;   // 2 bytes, the size of block before
  uint16_t free;    // 2 bytes, the status of the block
  uint16_t size;    // 2 bytes, the size (max 2^16/64Ki byte)
  struct head* next;  // 8 bytes pointer
  struct head* prev;  // 8 bytes pointer
};

struct head* after(struct head* block) {
  return (struct head*)((char*)block + (block->size + HEAD));
}

struct head* before(struct head* block) {
  return (struct head*)((char*)block - (block->bsize + HEAD));
}

struct head* split(struct head* block, int size) {
  int rsize = block->size - (size + HEAD);
  block->size = rsize;

  struct head* splt = after(block);
  splt->bsize = block->size;
  splt->bfree = block->free;
  splt->size = size;
  splt->free = FALSE;

  struct head* aft = after(splt);
  aft->bsize = splt->size;

  return splt;
}

struct head* arena = NULL;

struct head* new(){
  if (arena != NULL) {
    printf("one arena already allocated \n");
    return NULL;
  }

//using mmap, could use sbrk
  struct head* new = mmap(NULL, ARENA,
    PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
if (new == MAP_FAILED) {
  printf("mmap failed: error %d\n", errno);
  return NULL;
}

// make room for head and dummy
uint size = ARENA - 2*HEAD;

new->bfree = FALSE;
new->bsize = 0;
new->free = TRUE;
new->size = size;

struct head* sentinel = after(new);

// only touch the status fields
sentinel->bfree = new->free;
sentinel->bsize = new->size;
sentinel->free = FALSE;
sentinel->size = 0;

// this is the only arena we have
arena = (struct head*)new;

return new;
}

struct head* flist;

void detach(struct head* block) {
  if (block->next != NULL) {
    block->next->prev = block->prev;
  }
  if (block->prev != NULL) {
    block->prev->next = block->next;
  } else {
    flist = flist->next;
  }
}

void insert(struct head* block) {
  block->next = flist;
  block->prev = NULL;
  if (flist != NULL) {
    flist->prev = block;
  }
  flist = block;
}