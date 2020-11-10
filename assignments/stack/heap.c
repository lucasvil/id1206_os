#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

void zot(unsigned long *stop)
{
  unsigned long r = 0x3;

  unsigned long *i;
  for (i = &r; i <= stop; i++)
  {
    printf(" %p  0x%lx\n", i, *i);
  }
}

void foo(unsigned long *stop)
{
  unsigned long q = 0x2;
  zot(stop);
}

int main()
{
  int pid = getpid();
  char *heap = malloc(20);
  *heap = 0x61;
  printf("heap pointing to: 0x%x\n", *heap);
  free(heap);

  char *foo = malloc(20);
  *foo = 0x62;
  printf("foo pointing to: 0x%x\n", *foo);

  *heap = 0x63;
  printf("or is it pointing to: 0x%x\n", *foo);

  printf("\n\n /proc/%d/maps \n\n", pid);
  char command[50];
  sprintf(command, "cat /proc/%d/maps", pid);
  system(command);

  return 0;
}