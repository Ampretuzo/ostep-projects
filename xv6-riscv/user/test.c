#include "kernel/types.h"
#include "kernel/pstat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  struct pstat unused;
  unused.inuse[0] = 0;
  printf("Hello, world!\npstat.inuse[0] = %d\n", unused.inuse[0]);
  exit(0);
}
