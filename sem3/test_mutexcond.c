#include <stdio.h>
#include "green.h"

int flag = 0;
green_mutex_t mutex_g;
green_cond_t cond;

void* test(void* arg) {
  int id = *(int*)arg;
  int loop = 100;
  while (loop > 0) {
    printf("Thread %d: %d\n", id, loop);
    green_mutex_lock(&mutex_g);
    while (flag != id) {
      green_mutex_unlock(&mutex_g);
      green_cond_wait(&cond, &mutex_g);
    }
    flag = (id + 1) % 2;
    green_cond_signal(&cond);
    green_mutex_unlock(&mutex_g);
    loop--;
  }
}

int main() {
  green_t g0, g1;
  int a0 = 0;
  int a1 = 1;

  green_cond_init(&cond);
  green_mutex_init(&mutex_g);

  green_create(&g0, test, &a0);
  green_create(&g1, test, &a1);
  green_join(&g0, NULL);
  green_join(&g1, NULL);
  printf("done\n");
  return 0;
}