#include "headers.h"

void clearResources(int);

int main(int argc, char *argv[]) {
  signal(SIGINT, clearResources);

  initClk();

  while (true) {
    int tick = getClk();

    // TODO: setup the shared memory and semaphores
    // TODO: handle the processes that arrive

    while (tick == getClk()) {
      usleep(DELAY_TIME);
    }
  }

  destroyClk(false);
}

void clearResources(int signum) {
  destroyClk(false);
  exit(0);
}
