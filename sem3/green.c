#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <assert.h>
#include "green.h"

#define TRUE 1
#define FALSE 0

#define STACK_SIZE 4096

typedef struct queue {
  green_t* head;
  green_t* tail;
} queue;

static queue* ready_queue;

// add thread to tail of ready queue
static void enqueue(queue* queue, green_t* thread) {
  if ((queue->head == NULL) || (queue->tail == NULL)) {
    queue->head = thread;
    queue->tail = thread;
  } else {
    queue->tail->next = thread;
    queue->tail = thread;
  }
}

// dequeue thread at head of ready queue
static green_t* dequeue() {
  green_t* thread = ready_queue->head;
  ready_queue->head = thread->next;
  if (thread->next == NULL) {
    ready_queue->tail = NULL;
  } else {
    thread->next = NULL;
  }
  return thread;
}

static ucontext_t main_cntx = { 0 };
static green_t main_green = { &main_cntx, NULL, NULL, NULL, NULL, NULL, FALSE };

static green_t* running = &main_green;


static void init() __attribute__((constructor));

void init() {
  ready_queue = malloc(sizeof(queue));
  getcontext(&main_cntx);
}

void green_thread() {
  green_t* this = running;

  void* result = (this->fun)(this->arg);

  // place waiting (joining) thread in ready queue
  if (this->join) {
    enqueue(ready_queue, this->join);
  }

  // save result of execution
  this->retval = result;

  // we're a zombie
  this->zombie = TRUE;

  // find the next thread to run
  green_t* next = dequeue();
  running = next;
  setcontext(next->context);
}

int green_create(green_t* new, void* (*fun)(void*), void* arg) {
  ucontext_t* cntx = (ucontext_t*)malloc(sizeof(ucontext_t));
  getcontext(cntx);

  void* stack = malloc(STACK_SIZE);

  cntx->uc_stack.ss_sp = stack;
  cntx->uc_stack.ss_size = STACK_SIZE;
  makecontext(cntx, green_thread, 0);

  new->context = cntx;
  new->fun = fun;
  new->arg = arg;
  new->next = NULL;
  new->join = NULL;
  new->retval = NULL;
  new->zombie = FALSE;
  // add new to the ready queue;
  enqueue(ready_queue, new);

  return 0;
}

int green_yield() {
  green_t* susp = running;
  // add susp to ready queue
  enqueue(ready_queue, susp);

  // select the next thread for execution
  green_t* next = dequeue();
  running = next;
  swapcontext(susp->context, next->context);

  return 0;
}

int green_join(green_t* thread, void** res) {
  if (!thread->zombie) {
    // add as joining thread
    green_t* susp = running;
    thread->join = susp;

    //select the next thread for execution
    green_t* next = dequeue();
    running = next;
    swapcontext(susp->context, next->context);
  }
  // collect result
  res = thread->retval;

  // free context
  free(thread->context);

  return 0;
}