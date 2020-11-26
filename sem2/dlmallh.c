#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <errno.h>

#define TRUE 1
#define FALSE 0

#define HEAD (sizeof(struct taken))
#define MIN(size) (((size)>(16))?(size):(16))
#define LIMIT(size) (MIN(0) + HEAD + size)
#define MAGIC(memory)((struct taken*)memory - 1)
#define HIDE(block) (void*)((struct taken*)block + 1)
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

struct taken {
  uint16_t bfree;
  uint16_t bsize;
  uint16_t free;
  uint16_t size;
};

struct head* after(struct head* block) {
  return (struct head*)((char*)block + (block->size + HEAD));
}

struct head* before(struct head* block) {
  return (struct head*)((char*)block - (block->bsize + HEAD));
}

struct head* arena = NULL;

struct head* new(){
  if (arena != NULL) {
    printf("one arena already allocated \n");
    return NULL;
  }

/* using mmap, could use sbrk */
struct head* new = mmap(NULL, ARENA, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
if (new == MAP_FAILED) {
  printf("mmap failed: error %d\n", errno);
  return NULL;
}

/* make room for head and dummy */
uint size = ARENA - 2*HEAD;

new->bfree = FALSE;
new->bsize = 0;
new->free = TRUE;
new->size = size;

struct head* sentinel = after(new);

/* only touch the status fields */
sentinel->bfree = new->free;
sentinel->bsize = new->size;
sentinel->free = FALSE;
sentinel->size = 0;

/* this is the only arena we have */
arena = (struct head*)new;

return new;
}

struct head* flist;

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

/* detach from free list */
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

/* insert at flist */
void insert(struct head* block) {
  block->next = flist;
  block->prev = NULL;
  if (flist != NULL) {
    flist->prev = block;
  }
  flist = block;
}

/* adjust size to be multiple of ALIGN */
int adjust(size_t request) {
  int size = LIMIT(request);
  if (size % ALIGN) {
    return (size + ALIGN - (size % ALIGN));
  }
  return size;
}

/* find first free */
struct head* find(int size) {
  struct head* current = flist;
  while (current != NULL) {
    if (size <= current->size) {
      detach(current);
      if (current->size >= LIMIT(size)) {
        struct head* splt = split(current, size);
        insert(before(splt)); //add remaining back to list
        current = splt;
      }
      current->free = FALSE;
      after(current)->bfree = FALSE;
      return current;
    }
    current = current->next;
  }
  return NULL;
}

struct head* merge(struct head* block) {
  struct head* aft = after(block);
  if (block->bfree) {
    /* unlink the block before */
    detach(before(block));

    /* calculate and set the total size of the merged blocks */
    before(block)->size = block->size + before(block)->size + HEAD;

    /* update the block after the merged blocks */
    aft->bsize = before(block)->size;

    /* continue with the merged block */
    block = before(block);
  }

  if (aft->free) {
    /* unlink the block */
    detach(aft);

    /* calculate and set the total size of the merged blocks */
    block->size = block->size + aft->size + HEAD;

    /* update the block after the merged block */
    after(aft)->bsize = block->size;

  }
  return block;
}

void* dalloc(size_t request) {
  if (request <= 0) {
    return NULL;
  }
  if (arena == NULL) {
    arena = new();
    flist = arena;
  }
  int size = adjust(request);
  struct head* taken = find(size);
  if (taken == NULL) {
    return NULL;
  } else {
    return HIDE(taken);
  }
}

void dfree(void* memory) {
  if (memory != NULL) {
    struct head* block = MAGIC(memory);
    block = merge(block);
    struct head* aft = after(block);
    block->free = TRUE;
    aft->bfree = TRUE;
    insert(block);
  }
  return;
}

void printArena() {
  struct head* current = arena;
  while (current->size > 0) {
    printf("adress: %p \nsize: %d\nfree: %d\nbfree: %d\n", current, current->size, current->free, current->bfree);
    current = after(current);
  }
}

/* get length of free list */
int getFreeLength() {
  int length = 0;
  struct head* current = flist;
  while (current != NULL) {
    length++;
    current = current->next;
  }
  return length;
}

int getAllocSize() {
  int size = 0;
  struct head* current = arena;
  while (current->size > 0) {
    if (!current->free) {
      size += current->size;
    }
    current = after(current);
  }
  return size;
}

float flistAvg() {
  int count = 0;
  int totalSize = 0;
  struct head* current = flist;
  while (current != NULL) {
    totalSize += current->size;
    count++;
  }
  return ((float)totalSize / (float)count);
}

// TODO check for fragmentation
void sanity() {
  bool status = true;
  struct head* currentFree = flist;
  struct head* current = arena;
  bool bfree = current->bfree;
  int bsize = current->bsize;

  /* check arena is initialized correctly and intact */
  if ((current->bfree == TRUE) || (current->bsize != 0)) {
    status = false;
    printf("arena={bfree=%d, arena=%d}\n", current->bfree, current->bsize);
  }

  /* Check flist attributes */
  if (flist->bfree == TRUE) {
    status = false;
    printf("flist={bfree=%d, bszie=%p}\n", flist->bfree, flist);
  }

  /* loop list */
  while (current->size > 0) {
    if ((current->bfree != bfree) || (current->bsize != bsize)) {
      status = false;
      printf("Sanity: problem at adress: %p\n", current);
      printf("\t{bfree=%d, bsize=%d}\n", current->bfree, current->bsize);
      printf("\texpected={bfree=%d, bsize=%d}\n", bfree, bsize);
    }
    bfree = current->free;
    bsize = current->size;
    current = after(current);
  }
  if (status) {
    printf("Sanity: No problems found.\n");
  }
}