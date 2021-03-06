#include "headers.h"

void cont(int);

int remain;
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
  remain = atoi(argv[1]);

  down(procsemid);

  while (!started) {
    usleep(DELAY_TIME);
  }

  while (remain > 0) {
    int tick = getClk();
    --remain;
    while (tick == getClk()) {
      usleep(DELAY_TIME);
    }
  }

  // printf("Process %d died at %d\n", getpid(),
  //        getClk(), atoi(argv[1]));

  destroyClk(false);

  return 0;
}

void cont(int signum) {
  started = true;
}
