#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "green.h"

int flag = 0;
green_cond_t g_cond;
pthread_cond_t p_cond;
pthread_mutex_t p_mutex;

void* test(void* arg) {
  int id = *(int*)arg;
  int loop = 10000;
  while (loop > 0) {
    if (flag == id) {
      printf("thread %d: %d\n", id, loop);
      loop--;
      flag = (id + 1) %2;
      green_cond_signal(&g_cond);
    } else {
      green_cond_wait(&g_cond, NULL);
    }
  }
}

void* test_p(void* arg) {
  int id = *(int*)arg;
  int loop = 10000;
  while (loop > 0) {
    pthread_mutex_lock(&p_mutex);
    if (flag == id) {
      printf("thread %d: %d\n", id, loop);
      loop--;
      flag = (id + 1) % 2;
      pthread_cond_signal(&p_cond);
      pthread_mutex_unlock(&p_mutex);
    } else {
      //printf("thread %d: waiting\n", id);
      pthread_cond_wait(&p_cond, &p_mutex);
      pthread_mutex_unlock(&p_mutex);
    }
  }
}

int main() {
  int a0 = 0;
  int a1 = 1;
  int round = 0;
  int ms = 0;
  while (round < 100) {
    clock_t start = clock();

    /* green thread */
    // green_t g0, g1;
    // green_cond_init(&g_cond);
    // green_create(&g0, test, &a0);
    // green_create(&g1, test, &a1);
    // green_join(&g0, NULL);
    // green_join(&g1, NULL);

    /* pthread */
    pthread_t p0, p1;
    pthread_cond_init(&p_cond, NULL);
    pthread_mutex_init(&p_mutex, NULL);
    pthread_create(&p0, NULL, test_p, &a0);
    pthread_create(&p1, NULL, test_p, &a1);
    pthread_join(p0, NULL);
    pthread_join(p1, NULL);

    clock_t end = clock();
    ms += ((double)(end-start)/CLOCKS_PER_SEC) * 1000;
    printf("%d %d\n", round, ms);
    round++;
  }
  return 0;
}