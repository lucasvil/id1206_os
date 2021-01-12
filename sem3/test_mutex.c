#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "green.h"

volatile int count = 0;
green_mutex_t mutex_g;
pthread_mutex_t mutex_p;

void* test(void* arg) {
  int id = *(int*)arg;
  int loop = 100000;
  while (loop > 0) {
    green_mutex_lock(&mutex_g);
    loop--;
    count++;
    green_mutex_unlock(&mutex_g);
  }
}

void* test_p(void* arg) {
  int id = *(int*)arg;
  int loop = 100000;
  while (loop > 0) {
    pthread_mutex_lock(&mutex_p);
    loop--;
    count++;
    pthread_mutex_unlock(&mutex_p);
  }
}

int main() {
  int a0 = 0;
  int a1 = 1;
  int a2 = 2;
  int a3 = 3;
  int round = 0;
  double s = 0;
  while (round < 100) {

    clock_t start = clock();

    /* green */
    green_t g0, g1, g2, g3;
    green_mutex_init(&mutex_g);
    green_create(&g0, test, &a0);
    green_create(&g1, test, &a1);
    green_create(&g2, test, &a2);
    green_create(&g3, test, &a3);
    green_join(&g0, NULL);
    green_join(&g1, NULL);
    green_join(&g2, NULL);
    green_join(&g3, NULL);

    /* pthread */
    // pthread_t p0, p1, p2, p3;
    // pthread_mutex_init(&mutex_p, NULL);
    // pthread_create(&p0, NULL, test_p, &a0);
    // pthread_create(&p1, NULL, test_p, &a1);
    // pthread_create(&p2, NULL, test_p, &a2);
    // pthread_create(&p3, NULL, test_p, &a3);
    // pthread_join(p0, NULL);
    // pthread_join(p1, NULL);
    // pthread_join(p2, NULL);
    // pthread_join(p3, NULL);

    clock_t end = clock();
    s += ((double)(end-start)) / CLOCKS_PER_SEC;
    printf("%d %.2f\n", round, s);
    round++;
  }
  return 0;
}