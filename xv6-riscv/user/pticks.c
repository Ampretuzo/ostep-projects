#include "kernel/types.h"
#include "kernel/pstat.h"
#include "user/user.h"

void work() {
  int x = 1;
  while (1) {
    x = x * 2;  // Any better work for the poor cpu?
  }
}

int forkt(int ticks) {
  int rc = fork();
  if (rc == 0) {
    settickets(ticks);
    work();
    exit(0);
  } else {
    return rc;
  }
}

int gettickets(int pid, struct pstat *ps) {
  for (int i = 0; i < NPROC; i++) {
    if (ps->inuse[i] && ps->pid[i] == pid) {
      return ps->ticks[i];
    }
  }
  return -1;
}

/*
 * No error checking, no bs
 */
int
main(int argc, char *argv[])
{
  struct pstat ps;
  int ec = 0;
  int uptime0;

  int pid10 = forkt(10);
  int pid20 = forkt(20);
  int pid30 = forkt(30);
  printf("Forked pids: %d, %d, %d\n", pid10, pid20, pid30);

  printf("\n");
  printf("tickstotal, ticks10, ticks20, ticks30\n");
  uptime0 = uptime();
  while (1) {
    if (getpinfo(&ps) < 0) {
      printf("getpinfo returned nonzero\n");
      ec = 1;
      break;
    }
    int t10 = gettickets(pid10, &ps);
    int t20 = gettickets(pid20, &ps);
    int t30 = gettickets(pid30, &ps);
    printf("%d %d %d %d\n", uptime() - uptime0, t10, t20, t30);
    sleep(100);
  }

  kill(pid10);
  kill(pid20);
  kill(pid30);

  exit(ec);
}