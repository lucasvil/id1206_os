#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include "green.h"

#define PERIOD 10
#define TRUE 1
#define FALSE 0

#define STACK_SIZE 4096

static sigset_t block;

static queue* readyQ;

static ucontext_t main_cntx = { 0 };
static green_t main_green = { &main_cntx, NULL, NULL, NULL, NULL, NULL, FALSE };

static green_t* running = &main_green;

void timer_handler(int);

static void init() __attribute__((constructor));

void init() {
  readyQ = malloc(sizeof(queue));

  sigemptyset(&block);
  sigaddset(&block, SIGVTALRM);

  struct sigaction act = { 0 };
  struct timeval interval;
  struct itimerval period;

  act.sa_handler = timer_handler;
  assert(sigaction(SIGVTALRM, &act, NULL) == 0);
  interval.tv_sec = 0;
  interval.tv_usec = PERIOD;
  period.it_interval = interval;
  period.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &period, NULL);

  getcontext(&main_cntx);
}

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
static green_t* dequeue(queue* queue) {
  green_t* thread = queue->head;
  queue->head = thread->next;
  if (thread->next == NULL) {
    queue->tail = NULL;
  } else {
    thread->next = NULL;
  }
  return thread;
}

void timer_handler(int sig) {
  sigprocmask(SIG_BLOCK, &block, NULL);
  green_t* susp = running;

  // add the running to the ready queue
  enqueue(readyQ, susp);

  // find the next thread for execution
  green_t* next = dequeue(readyQ);
  running = next;
  swapcontext(susp->context, next->context);
  sigprocmask(SIG_UNBLOCK, &block, NULL);

}

void green_cond_init(green_cond_t* cond) {
  cond->suspQ = malloc(sizeof(queue));
}

void green_cond_wait(green_cond_t* cond) {
  green_t* susp = running;

  sigprocmask(SIG_BLOCK, &block, NULL);
  enqueue(cond->suspQ, susp);

  green_t* next = dequeue(readyQ);
  running = next;
  swapcontext(susp->context, next->context);
  sigprocmask(SIG_UNBLOCK, &block, NULL);
}

void green_cond_signal(green_cond_t* cond) {
  if (!(cond->suspQ->head == NULL)) {
    sigprocmask(SIG_BLOCK, &block, NULL);
    green_t* unsusp = dequeue(cond->suspQ);
    enqueue(readyQ, unsusp);
    sigprocmask(SIG_UNBLOCK, &block, NULL);
  }
}

void green_thread() {
  green_t* this = running;

  void* result = (this->fun)(this->arg);

  // place waiting (joining) thread in ready queue
  sigprocmask(SIG_BLOCK, &block, NULL);
  if (this->join) {
    enqueue(readyQ, this->join);
  }

  // save result of execution
  this->retval = result;

  // we're a zombie
  this->zombie = TRUE;

  // find the next thread to run
  green_t* next = dequeue(readyQ);
  running = next;
  setcontext(next->context);
  sigprocmask(SIG_UNBLOCK, &block, NULL);
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
  sigprocmask(SIG_BLOCK, &block, NULL);
  enqueue(readyQ, new);
  sigprocmask(SIG_UNBLOCK, &block, NULL);

  return 0;
}

int green_yield() {
  green_t* susp = running;
  // add susp to ready queue
  sigprocmask(SIG_BLOCK, &block, NULL);
  enqueue(readyQ, susp);

  // select the next thread for execution
  green_t* next = dequeue(readyQ);
  running = next;
  swapcontext(susp->context, next->context);
  sigprocmask(SIG_UNBLOCK, &block, NULL);

  return 0;
}

int green_join(green_t* thread, void** res) {
  if (!thread->zombie) {
    // add as joining thread
    green_t* susp = running;
    thread->join = susp;

    //select the next thread for execution
    sigprocmask(SIG_BLOCK, &block, NULL);
    green_t* next = dequeue(readyQ);
    running = next;
    swapcontext(susp->context, next->context);
    sigprocmask(SIG_UNBLOCK, &block, NULL);
  }
  // collect result
  res = thread->retval;

  // free context
  free(thread->context);

  return 0;
}