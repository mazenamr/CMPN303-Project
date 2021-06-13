#include "headers.h"

void cont(int);

int remainingTime;
int startTime;
int waitTime = 0;
int tick;
bool started = false;

int main(int argc, char *argv[]) {
  signal(SIGCONT, cont);

  int procsemid = semget(PROCSEMKEY, 1, 0444);
  while ((int)procsemid == -1) {
    printf("Wait! The semaphore not initialized yet!\n");
    sleep(1);
    procsemid = semget(PROCSEMKEY, 1, 0444);
  }

  if (argc < 2) {
    down(procsemid);
    printf("Too few arguments!\n");
    exit(-1);
  }

  initClk();
  tick = getClk();
  remainingTime = atoi(argv[1]);

  down(procsemid);

  while (!started) {
    tick = getClk();
    usleep(DELAY_TIME);
  }

  startTime = getClk();

  while (remainingTime > 0) {
    tick = getClk();
    --remainingTime;
    while (tick == getClk()) {
      usleep(DELAY_TIME);
    }
  }

  printf("Process %d died at %d with run time %d and wait time %d\n", getpid(),
         tick, atoi(argv[1]), waitTime);

  destroyClk(false);

  down(procsemid);

  return 0;
}

void cont(int signum) {
  started = true;
  waitTime += (getClk() - tick);
}
