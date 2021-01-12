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

static green_queue* ready_queue;

static ucontext_t main_cntx = { 0 };
static green_t main_green = { &main_cntx, NULL, NULL, NULL, NULL, NULL, FALSE };

static green_t* running = &main_green;

void timer_handler(int);

static void init() __attribute__((constructor));

/*
* QUEUE
*/

green_queue* newQueue() {
  green_queue* queue = malloc(sizeof(queue));
  queue->head = NULL;
  queue->tail = NULL;
  queue->size = 0;
  return queue;
}

// add thread to tail of ready queue
void enqueue(green_queue* queue, green_t* thread) {
  if (thread != NULL) {
    if ((queue->head == NULL) || (queue->tail == NULL)) {
      queue->head = thread;
      queue->tail = thread;
    } else {
      queue->tail->next = thread;
      queue->tail = thread;
    }
    queue->size++;
  }
}
// dequeue thread at head of ready queue
green_t* dequeue(green_queue* queue) {
  if (queue->head == NULL) {
    queue->tail = NULL;
    return NULL;
  }

  green_t* thread = queue->head;
  queue->head = queue->head->next;
  if (queue->head == NULL) {
    queue->tail = NULL;
  } else {
    thread->next = NULL;
  }
  queue->size--;
  return thread;
}

/*
* GREEN
*/

void init() {
  ready_queue = (green_queue*)newQueue();

  getcontext(&main_cntx);
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
  green_t* next = dequeue(ready_queue);
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
  sigprocmask(SIG_BLOCK, &block, NULL);

  green_t* susp = running;
  // add susp to ready queue
  enqueue(ready_queue, susp);

  // select the next thread for execution
  green_t* next = dequeue(ready_queue);
  running = next;
  swapcontext(susp->context, next->context);

  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

int green_join(green_t* thread, void** res) {
  sigprocmask(SIG_BLOCK, &block, NULL);
  if (!thread->zombie) {
    // add as joining thread
    green_t* susp = running;
    thread->join = susp;

    //select the next thread for execution
    green_t* next = dequeue(ready_queue);
    running = next;
    swapcontext(susp->context, next->context);
  }
  // collect result
  res = thread->retval;

  // free context
  free(thread->context);

  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

/*
* TIMER
*/

void timer_handler(int sig) {
  green_t* susp = running;

  // add the running to the ready queue
  enqueue(ready_queue, susp);

  // find the next thread for execution
  green_t* next = dequeue(ready_queue);
  running = next;
  swapcontext(susp->context, next->context);
}

/*
* COND
*/

void green_cond_init(green_cond_t* cond) {
  cond->susp = (green_queue*)newQueue();;
}

int green_cond_wait(green_cond_t* cond, green_mutex_t* mutex) {
  sigprocmask(SIG_BLOCK, &block, NULL);

  // suspend the running thread on condition
  green_t* susp = running;
  enqueue(cond->susp, susp);

  if (mutex != NULL) {
    // release the lock if we have a mutex
    green_mutex_unlock(mutex);

    // move suspended thread to the ready queue
    green_t* thread = dequeue(mutex->susp);
    enqueue(ready_queue, thread);
  }

  // schedule next thread
  green_t* next = dequeue(ready_queue);
  running = next;
  swapcontext(susp->context, next->context);

  if (mutex != NULL) {
    // try to take the lock
    if (mutex->taken) {
      // bad luck, suspend
      susp = running;
      next = dequeue(ready_queue);
      swapcontext(susp->context, next->context);
    } else {
      // take the lock
      mutex->taken = TRUE;
    }
  }
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

// void green_cond_wait(green_cond_t* cond) {
//   sigprocmask(SIG_BLOCK, &block, NULL);
//   green_t* susp = running;
//   enqueue(cond->susp, susp);

//   green_t* next = dequeue(ready_queue);
//   running = next;
//   swapcontext(susp->context, next->context);
//   sigprocmask(SIG_UNBLOCK, &block, NULL);
// }

void green_cond_signal(green_cond_t* cond) {
  if (cond->susp->size > 0) {
    green_t* susp = dequeue(cond->susp);
    enqueue(ready_queue, susp);
  }
}

/*
* MUTEX
*/

int green_mutex_init(green_mutex_t* mutex) {
  mutex->taken = FALSE;
  mutex->susp = (green_queue*)newQueue();
  return 0;
}

int green_mutex_lock(green_mutex_t* mutex) {
  sigprocmask(SIG_BLOCK, &block, NULL);

  green_t* susp = running;
  if (mutex->taken) {
    // suspend the running thread
    enqueue(mutex->susp, susp);

    // find the next thread
    green_t* next = dequeue(ready_queue);
    running = next;
    swapcontext(susp->context, next->context);
  } else {
    // take the lock
    mutex->taken = TRUE;
  }
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

int green_mutex_unlock(green_mutex_t* mutex) {
  sigprocmask(SIG_BLOCK, &block, NULL);
  if (mutex->susp->size > 0) {
    // move suspended thread to ready queue
    green_t* susp = dequeue(mutex->susp);
    enqueue(ready_queue, susp);
  } else {
    // release lock
    mutex->taken = FALSE;
  }
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}