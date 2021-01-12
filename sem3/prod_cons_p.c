#include <stdio.h>
#include <pthread.h>

#define BSIZE 4

int* buffer[BSIZE];
int count = 0;
pthread_mutex_t mutex_p;
pthread_cond_t cond_p;

void produce(int i) {
  int* prod = rand();
  printf("PRODUCING! count=%d i=%d\n", count, i);
  buffer[i % BSIZE] = prod;
  count++;
}

void consume(int i) {
  printf("CONSUMING! count=%d i=%d\n", count, i);
  buffer[i % BSIZE] = NULL;
  count--;
}

void* producer() {
  for (int i = 0; i < 100; i++) {
    pthread_mutex_lock(&mutex_p);
    if (count == BSIZE) {
      pthread_cond_wait(&cond_p, &mutex_p);
    }
    produce(i);
    if (count == 1) {
      pthread_cond_signal(&cond_p);
    }
    pthread_mutex_unlock(&mutex_p);
  }
}

void* consumer() {
  for (int i = 0; i < 100; i++) {
    pthread_mutex_lock(&mutex_p);
    if (count == 0) {
      pthread_cond_wait(&cond_p, &mutex_p);
    }
    consume(i);
    if (count == BSIZE - 1) {
      pthread_cond_signal(&cond_p);
    }
    pthread_mutex_unlock(&mutex_p);
  }
}

int main()
{
  pthread_t p, c;
  printf("started\n");
  pthread_cond_init(&cond_p, NULL);
  pthread_mutex_init(&mutex_p, NULL);
  pthread_create(&p, NULL, producer, NULL);
  pthread_create(&c, NULL, consumer, NULL);
  pthread_join(p, NULL);
  pthread_join(c, NULL);
  printf("done\n");
}