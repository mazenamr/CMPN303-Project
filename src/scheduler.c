#include "headers.h"

void clearResources(int);
static inline void setupIPC();
ProcessInfo addProcess(Process*);
void startProcess(ProcessInfo*);
void removeProcess(ProcessInfo*);

void fcfs();
void sjf();
void hpf();
void srtn();
void rr();

int shmid;
int semid;
int *bufferaddr;

PCB *processTable[PROCESS_TABLE_SIZE];
ProcessInfo *runningProcess = NULL;
Deque *arrived = NULL;                  // equvialent to the ready queue 
Deque *deque = NULL;
PriorityQueue *priorityQueue = NULL;
CircularQueue *circularQueue = NULL;
int counter = 0;

int main(int argc, char *argv[]) {
  setupIPC();

  int *messageCount = (int *)bufferaddr;
  Process *buffer = (Process *)((void *)bufferaddr + sizeof(int));

  signal(SIGINT, clearResources);

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

  arrived = newDeque(sizeof(PCB));

  printf("#At\ttime\tx\tprocess\ty\tstate\t"
         "\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");
  while (true) {
    int tick = getClk();

    down(semid);
    for (int i = 0; i < *messageCount; ++i) {
      Process *currentProcess = buffer + i;
      ProcessInfo newProcess = addProcess(currentProcess);
      pushBack(arrived, &newProcess);
    }
    *messageCount = 0;
    up(semid);

    switch (sch) {
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
  PCB *pcb;
  Node *process = NULL;

  if (deque == NULL) {
    deque = newDeque(sizeof(Process));
  }

  while (popFront(arrived, (void **)&process)) {
    pushBack(deque, process);
  }

  if (deque->head == NULL) {
    return;
  }

  if (runningProcess == NULL) {
    if (!peekFront(deque, (void **)&runningProcess)) {
      return;
    }
    startProcess(runningProcess);
  }

  pcb = processTable[runningProcess->id];
  pcb->remainingTime -= 1;
  pcb->executionTime += 1;

  if (!pcb->remainingTime) {
    removeFront(deque);
    removeProcess(runningProcess);
    runningProcess = NULL;
  } else {
    process = deque->head->next;
    while (process != NULL) {
      int id = ((Process *)(process->data))->id;
      processTable[id]->waitingTime += 1;
      process = process->next;
    }
    free(process);
  }
}

void sjf() {
  PCB *pcb;
  PriorityNode *process = NULL; 

  if (priorityQueue == NULL) {
    priorityQueue = newPriorityQueue(sizeof(process));
  }

  while (popFront(arrived, (void **)&process)) {
    counter++;
    enqueuePQ(priorityQueue, process, -processTable[counter]->runtime);
  }

  //printf("check 3 \n");
  if (priorityQueue->head == NULL) {
    return;
  }

  if (runningProcess == NULL) {
    if (!peekPQ(priorityQueue, (void **)&runningProcess)) {
      return;
    }
    startProcess(runningProcess);
  }

  pcb = processTable[runningProcess->id];
  pcb->remainingTime -= 1;
  pcb->executionTime += 1;

  if (!pcb->remainingTime) {
    dequeuePQ(priorityQueue, (void**)&process);
    removeProcess(runningProcess);
    runningProcess = NULL;
  } else {
    process = priorityQueue->head->next;
    while (process!= NULL) {
      int id = ((Process *)(process->data))->id;
      processTable[id]->waitingTime += 1;
      process = process->next;
    }
    free(process);
  }
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

ProcessInfo addProcess(Process *process) {
  processTable[process->id] = malloc(sizeof(PCB));
  PCB *pcb = processTable[process->id];
  pcb->id = process->id;
  pcb->arrival = process->arrival;
  pcb->runtime = process->runtime;
  pcb->priority = process->priority;
  pcb->remainingTime = process->runtime;
  pcb->executionTime = 0;
  pcb->waitingTime = 0;
  char runtime[8];
  sprintf(runtime, "%d", process->runtime);
  pid_t pid = fork();
  if (!pid) {
    execl("bin/process.out", "process.out", runtime, NULL);
  }
  kill(pid, SIGSTOP);
  ProcessInfo newProcess;
  newProcess.id = process->id;
  newProcess.pid = pid;
  return newProcess;
}

void startProcess(ProcessInfo *process) {
  kill(process->pid, SIGCONT);
  PCB *pcb = processTable[process->id];
  pcb->startTime = getClk();
  printf("At\ttime\t%d\tprocess\t%d\tstarted\t"
         "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
         getClk(), process->id, pcb->arrival, pcb->runtime, pcb->remainingTime,
         pcb->waitingTime);
}

void removeProcess(ProcessInfo *process) {
  PCB *pcb = processTable[process->id];
  pcb->startTime = getClk();
  printf("At\ttime\t%d\tprocess\t%d\tfinished"
         "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
         getClk(), process->id, pcb->arrival, pcb->runtime, pcb->remainingTime,
         pcb->waitingTime);
  free(pcb);
}

void clearResources(int signum) {
  // clear resources
  // but only if they weren't already cleared
  static bool ended = false;
  if (!ended) {
    ended = true;
    if (arrived != NULL) {
      deleteDeque(arrived);
    }
    if (deque != NULL) {
      deleteDeque(deque);
    }
    if (priorityQueue != NULL) {
      deletePriorityQueue(priorityQueue);
    }
    if (circularQueue != NULL) {
      deleteCircularQueue(circularQueue);
    }
    shmdt(bufferaddr);
    destroyClk(false);
  }
  exit(0);
}
