#include <ucontext.h>

typedef struct green_t {
  ucontext_t* context;
  void* (*fun)(void*);
  void* arg;
  struct green_t* next;
  struct green_t* join;
  void* retval;
  int zombie;
} green_t;

typedef struct queue {
  green_t* head;
  green_t* tail;
} queue;

typedef struct green_cond_t {
  struct queue* suspQ;
} green_cond_t;


int green_create(green_t* thread, void* (*fun)(void*), void* arg);
int green_yield();
int green_join(green_t* thread, void** val);
void green_cond_init(green_cond_t* cond);
void green_cond_wait(green_cond_t* cond);
void green_cond_signal(green_cond_t* cond);