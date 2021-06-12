#include "headers.h"

void cont(int);
void stop(int);

int remainingTime;
int startTime;
int waitTime = 0;
int stoppedTime;
int var = 0;
bool started = false;

int main(int argc, char *argv[]) {
  started = false;
  var = atoi(argv[1]);
  initClk();

  signal(SIGCONT, cont);
  signal(SIGSTOP, stop);

  if (argc < 2) {
    printf("Too few arguments!\n");
    exit(-1);
  }

  remainingTime = atoi(argv[1]);
  printf("remaining time is %d\n", remainingTime);

  while (!started) {
    usleep(DELAY_TIME);
  }
  printf("Awake!\n");

  startTime = getClk();

  while (remainingTime > 0) {
    int tick = getClk();
    remainingTime = getClk() - remainingTime;
    while (tick == getClk()) {
      usleep(DELAY_TIME);
    }
    printf("Rem Time %d\n", remainingTime);
  }

  printf("Process %d died at %d with run time %d and wait time %d\n", getpid(),
         getClk(), atoi(argv[1]), waitTime);

  destroyClk(false);

  return 0;
}

void cont(int signum) {
  printf("Continued \n");
  started = true;
  waitTime += getClk() - stoppedTime;
  remainingTime = var;
}

void stop(int signum) { stoppedTime = getClk(); }
