#include "headers.h"

void clearResources(int);
static inline void setupIPC();
int shmid;
int semid;
int *shmaddr;

int main(int argc, char *argv[]) {
  signal(SIGINT, clearResources);

  initClk();

  setupIPC();

  while (true) {
    int tick = getClk();

    // TODO: setup the shared memory and semaphores
    // TODO: handle the processes that arrive

    while (tick == getClk()) {
      usleep(DELAY_TIME);
    }
  }

  destroyClk(false);
static inline void setupIPC() {
  shmid = shmget(BUFKEY, sizeof(int) + sizeof(Process) * BUFFER_SIZE, 0444);
  while ((int)shmid == -1) {
    printf("Wait! The buffer not initialized yet!\n");
    sleep(1);
    shmid = shmget(BUFKEY, sizeof(int) + sizeof(Process) * BUFFER_SIZE, 0444);
  }

  shmaddr = (int *)shmat(shmid, (void *)0, 0);
  if ((long)shmaddr == -1) {
    perror("Error in attaching the buffer in scheduler!");
    exit(-1);
  }

  semid = semget(SEMKEY, 1, 0444);
  while ((int)shmid == -1) {
    printf("Wait! The semaphore not initialized yet!\n");
    sleep(1);
    semid = semget(SEMKEY, 1, 0444);
  }
}

void clearResources(int signum) {
  destroyClk(false);
  exit(0);
}
