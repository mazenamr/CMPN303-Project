#include "headers.h"

void clearResources(int);
static inline void setupIPC();
void fcfs();
void sjf();
void hpf();
void srtn();
void rr();

int shmid;
int semid;
int *shmaddr;

int main(int argc, char *argv[]) {
  signal(SIGINT, clearResources);

  initClk();

  setupIPC();

  if (argc < 2) {
    printf("No scheduling algorithm provided!\n");
    printSchedulingAlgorithms();
    exit(-1);
  }

  if (strlen(argv[1]) > 1) {
    printf("Invalid scheduling algorithm!\n");
    printSchedulingAlgorithms();
    exit(-1);
  }

  if (!isdigit(argv[1][0])) {
    printf("Invalid scheduling algorithm!\n");
    printSchedulingAlgorithms();
    exit(-1);
  }

  if (atoi(argv[1]) < FCFS || atoi(argv[2]) > RR) {
    printf("Invalid scheduling algorithm!\n");
    printSchedulingAlgorithms();
    exit(-1);
  }

  switch (atoi(argv[1])) {
  case FCFS:
    fcfs();
    break;
  case SJF:
    sjf();
    break;
  case HPF:
    hpf();
    break;
  case SRTN:
    srtn();
    break;
  case RR:
    rr();
    break;
  default:
    printf("Invalid scheduling algorithm!\n");
    printSchedulingAlgorithms();
    exit(-1);
  }

  destroyClk(false);
}

void fcfs() {
  while (true) {
    int tick = getClk();

    while (tick == getClk()) {
      usleep(DELAY_TIME);
    }
  }
}

void sjf() {
  while (true) {
    int tick = getClk();

    while (tick == getClk()) {
      usleep(DELAY_TIME);
    }
  }
}

void hpf() {
  while (true) {
    int tick = getClk();

    while (tick == getClk()) {
      usleep(DELAY_TIME);
    }
  }
}

void srtn() {
  while (true) {
    int tick = getClk();

    while (tick == getClk()) {
      usleep(DELAY_TIME);
    }
  }
}

void rr() {
  while (true) {
    int tick = getClk();

    while (tick == getClk()) {
      usleep(DELAY_TIME);
    }
  }
}

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
