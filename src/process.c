#include "headers.h"

void cont(int);
void stop(int);

int remainingTime;
int startTime;
int waitTime ;
int stoppedTime = 0;
bool started = false;

int main(int argc, char *argv[]) {
  initClk();

  signal(SIGCONT, cont);
  signal(SIGSTOP, stop);

  if (argc < 2) {
    printf("Too few arguments!\n");
    exit(-1);
  }

  remainingTime = atoi(argv[1]);

  while (!started) {
    usleep(DELAY_TIME);
  }

  startTime = getClk();

  while (remainingTime > 0) {
    int tick = getClk();
    remainingTime = getClk() - remainingTime;
    while (tick == getClk()) {
      usleep(DELAY_TIME);
    }
  }

  printf("Process %d died at %d with run time %d and wait time %d\n", getpid(),
         getClk(), atoi(argv[1]), waitTime);

  destroyClk(false);

  return 0;
}

void cont(int signum) {
  started = true;
  waitTime += getClk() - stoppedTime;
}

void stop(int signum) { stoppedTime = getClk(); }
