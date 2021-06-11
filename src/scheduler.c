#include "headers.h"

void clearResources(int);
static inline void setupIPC();
void fcfs();
void sjf();
void hpf();
void srtn();
void rr();
void addPCB(Process*p);

int shmid;
int semid;
int *bufferaddr;
PCB *processTable[PROCESS_TABLE_SIZE];
int *messageCount;
Process *buffer;

int main(int argc, char *argv[]) {
  signal(SIGINT, clearResources);

  setupIPC();
  messageCount = (int *)bufferaddr;
  buffer = (Process *)((void *)bufferaddr + sizeof(int));

  initClk();

  if (argc < 2) {
    printf("No scheduling algorithm provided!\n");
    printSchedulingAlgorithms();
    exit(-1);
  }

  SCHEDULING_ALGORITHM sch = atoi(argv[1]);

  if (sch < FCFS || sch > RR) {
    printf("Invalid scheduling algorithm!\n");
    printSchedulingAlgorithms();
    exit(-1);
  }

  printf("#id\tarrival\ttime\truntime\tpriority\n");
   while (true) {                                                 // Main loop 
    int tick = getClk();
    down(semid);
    for (int i = 0; i < *messageCount; ++i) {
      Process *currentProcess = buffer + i;
      printf("%d\t%d\t%d\t%d\t%d\n", currentProcess->id,
             currentProcess->arrival, getClk(), currentProcess->runtime,
             currentProcess->priority);
      addPCB(currentProcess);
    }
    *messageCount = 0;
    up(semid);
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
    while (tick == getClk()) {
      down(semid);
      if (*messageCount) {
        up(semid);
        break;
      }
      up(semid);
      usleep(DELAY_TIME);
    }
  }

  clearResources(-1);
}

void fcfs() {
}

void sjf() {
}

void hpf() {
}

void srtn(){
}

void rr() {
}

static inline void setupIPC() {
  shmid = shmget(BUFKEY, sizeof(int) + sizeof(Process) * BUFFER_SIZE, 0444);
  while ((int)shmid == -1) {
    printf("Wait! The buffer not initialized yet!\n");
    sleep(1);
    shmid = shmget(BUFKEY, sizeof(int) + sizeof(Process) * BUFFER_SIZE, 0444);
  }

  bufferaddr = (int *)shmat(shmid, (void *)0, 0);
  if ((long)bufferaddr == -1) {
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

void addPCB(Process *p){
  processTable[p->id] = malloc(sizeof(PCB));
  processTable[p->id]->id = p->id;
  processTable[p->id]->arrival = p->arrival;
  processTable[p->id]->runtime = p->runtime;
  processTable[p->id]->priority = p->priority;
  processTable[p->id]->waitingtime = 0;
  processTable[p->id]->starttime = -1;
  processTable[p->id]->remainingtime = p->runtime;
}

void clearResources(int signum) {
  // clear resources
  // but only if they weren't already cleared
  static bool ended = false;
  if (!ended) {
    ended = true;
    destroyClk(true);
  }
  exit(0);
}
