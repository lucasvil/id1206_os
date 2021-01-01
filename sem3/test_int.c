#include <stdio.h>
#include "green.h"

void* test(void* arg) {
  char id = '0' + *(int*)arg;
  int loop = 10000;
  while (loop > 0) {
    char str[] = "Thread:  \n";
    str[8] = id;
    write(1, str, sizeof(str) - 1);
    loop--;
  }
}

int main() {
  green_t g0, g1;
  int a0 = 0;
  int a1 = 1;

  printf("creating threads\n");
  green_create(&g0, test, &a0);
  green_create(&g1, test, &a1);
  green_join(&g0, NULL);
  green_join(&g1, NULL);
  printf("done\n");
  return 0;
}