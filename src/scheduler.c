#include "headers.h"
#include <unistd.h>

static inline void setupIPC();
static inline void loadBuffer();

ProcessInfo addProcess(Process*);
void startProcess(ProcessInfo*);
void resumeProcess(ProcessInfo*);
void stopProcess(ProcessInfo*);
void removeProcess(ProcessInfo*);

bool fcfs();
void sjf();
void hpf();
void srtn();
void rr();

void clearResources(int);

int tick;

int shmid;
int bufsemid;
int procsemid;
int *bufferaddr;

int *messageCount;
Process *buffer;

Deque *arrived = NULL;

Deque *deque = NULL;
PriorityQueue *priorityQueue = NULL;
CircularQueue *circularQueue = NULL;

ProcessInfo *runningProcess = NULL;
PCB **processTable = NULL;
int processTableSize = PROCESS_TABLE_SIZE;

int main(int argc, char *argv[]) {
  setupIPC();

  messageCount = (int *)bufferaddr;
  buffer = (Process *)((void *)bufferaddr + sizeof(int));

  processTable = malloc(processTableSize * sizeof(PCB *));

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

  bool ran = false;
  printf("#At\ttime\tx\tprocess\ty\tstate\t"
         "\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n\n");
  while (true) {
    loadBuffer();
    tick = getClk();

    if (!ran) {
      switch (sch) {
      case FCFS:
        ran = fcfs();
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
    }

    while (true) {
      down(bufsemid);
      if (*messageCount) {
        up(bufsemid);
        break;
      }
      up(bufsemid);
      usleep(DELAY_TIME);
      if (tick != getClk()) {
        ran = false;
        break;
      }
    }
  }
  clearResources(-1);
}

static inline void setupIPC() {
  semun s;
  s.val = 1;

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

  bufsemid = semget(BUFSEMKEY, 1, 0444);
  while ((int)bufsemid == -1) {
    printf("Wait! The semaphore not initialized yet!\n");
    sleep(1);
    bufsemid = semget(BUFSEMKEY, 1, 0444);
  }

  procsemid = semget(PROCSEMKEY, 1, 0644 | IPC_CREAT);
  if ((int)procsemid == -1) {
    perror("Error in creating semaphore!");
    exit(-1);
  }

  if ((int)semctl(procsemid, 0, SETVAL, s) == -1) {
    perror("Error in semctl!");
    exit(-1);
  }
}

static inline void loadBuffer() {
    down(bufsemid);
    for (int i = 0; i < *messageCount; ++i) {
      Process *currentProcess = buffer + i;
      if (currentProcess->id >= processTableSize) {
        int oldSize = processTableSize;
        while (currentProcess->id >= processTableSize) {
          processTableSize *= 2;
        }
        PCB **newProcessTable = malloc(processTableSize * sizeof(PCB *));
        memcpy(newProcessTable, processTable, oldSize * sizeof(PCB *));
        free(processTable);
        processTable = newProcessTable;
      }
      ProcessInfo newProcess = addProcess(currentProcess);
      pushBack(arrived, &newProcess);
    }
    *messageCount = 0;
    up(bufsemid);
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
  // we need to make sure the process has
  // properly started before we stop it
  waitzero(procsemid);
  kill(pid, SIGSTOP);
  up(procsemid);
  ProcessInfo newProcess;
  newProcess.id = process->id;
  newProcess.pid = pid;
  return newProcess;
}

void startProcess(ProcessInfo *process) {
  kill(process->pid, SIGCONT);
  PCB *pcb = processTable[process->id];
  pcb->startTime = tick;
  printf("At\ttime\t%d\tprocess\t%d\tstarted\t"
         "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
         tick, process->id, pcb->arrival, pcb->runtime, pcb->remainingTime,
         pcb->waitingTime);
}

void resumeProcess(ProcessInfo *process) {
  kill(process->pid, SIGCONT);
  PCB *pcb = processTable[process->id];
  printf("At\ttime\t%d\tprocess\t%d\tresumed\t"
         "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
         tick, process->id, pcb->arrival, pcb->runtime, pcb->remainingTime,
         pcb->waitingTime);
}

void stopProcess(ProcessInfo *process) {
  kill(process->pid, SIGSTOP);
  PCB *pcb = processTable[process->id];
  printf("At\ttime\t%d\tprocess\t%d\tresumed\t"
         "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
         tick, process->id, pcb->arrival, pcb->runtime, pcb->remainingTime,
         pcb->waitingTime);
}

void removeProcess(ProcessInfo *process) {
  // improve synchronization by waiting until
  // the process has really ended before removing it
  waitzero(procsemid);
  up(procsemid);
  PCB *pcb = processTable[process->id];
  pcb->startTime = tick;
  printf("At\ttime\t%d\tprocess\t%d\tfinished"
         "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n\n",
         tick, process->id, pcb->arrival, pcb->runtime, pcb->remainingTime,
         pcb->waitingTime);
  free(pcb);
}

bool fcfs() {
  PCB *pcb;
  // initialize deque of project info if it's not initialized
  if (deque == NULL) {
    deque = newDeque(sizeof(ProcessInfo));
  }

  // load the newly arrived processes into the deque
  ProcessInfo *processInfo = NULL;
  while (popFront(arrived, (void **)&processInfo)) {
    pushBack(deque, processInfo);
  }
  free(processInfo);

  // if the deque is empty, there is nothing to do
  if (deque->head == NULL) {
    return false;
  }

  // if we don't have a running process then we
  // should start the next process in the deque
  while (runningProcess == NULL) {
    // if we don't have a running process
    // and the deque is empty, then there
    // is nothing to do
    if (!peekFront(deque, (void **)&runningProcess)) {
      return false;
    }

    startProcess(runningProcess);

    // if the process that we have just started
    // has a runtime of zero then we should
    // remove it and try to load another one
    pcb = processTable[runningProcess->id];
    if (pcb->remainingTime == 0) {
      removeFront(deque);
      removeProcess(runningProcess);
      free(runningProcess);
      runningProcess = NULL;
      continue;
    }
    break;
  }

  // the head of the deque should be
  // the currently running process
  // so we will increase the wait time
  // of all the processes after the head
  Node *process = deque->head->next;
  while (process != NULL) {
    int id = ((ProcessInfo *)(process->data))->id;
    processTable[id]->waitingTime += 1;
    process = process->next;
  }

  // update the pcb of the running process
  pcb = processTable[runningProcess->id];
  pcb->remainingTime -= 1;
  pcb->executionTime += 1;

  // if the running process has finished
  // then we need to remove it
  if (runningProcess != NULL) {
    pcb = processTable[runningProcess->id];
    if (!pcb->remainingTime) {
      removeFront(deque);
      removeProcess(runningProcess);
      free(runningProcess);
      runningProcess = NULL;
    }
    return true;
  }
  return false;
}

void sjf() {
  if (priorityQueue == NULL) {
    priorityQueue = newPriorityQueue(sizeof(ProcessInfo));
  }

  ProcessInfo *processInfo = NULL;
  while (popFront(arrived, (void **)&processInfo)) {
    enqueuePQ(priorityQueue, processInfo,
              -processTable[processInfo->id]->runtime);
  }
  free(processInfo);

  if (priorityQueue->head != NULL) {
    PriorityNode *process = priorityQueue->head->next;
    while (process != NULL) {
      int id = ((ProcessInfo *)(process->data))->id;
      processTable[id]->waitingTime += 1;
      process = process->next;
    }
  } else {
    return;
  }

  if (runningProcess == NULL) {
    if (!peekPQ(priorityQueue, (void **)&runningProcess)) {
      return;
    }
    startProcess(runningProcess);
  }

  PCB *pcb = processTable[runningProcess->id];

  if (!pcb->remainingTime) {
    removePQ(priorityQueue);
    removeProcess(runningProcess);
    runningProcess = NULL;
  } else {
    pcb->remainingTime -= 1;
    pcb->executionTime += 1;
  }
}

void hpf() {
  if (priorityQueue == NULL) {
    priorityQueue = newPriorityQueue(sizeof(ProcessInfo));
  }

  ProcessInfo *processInfo = NULL;
  while (popFront(arrived, (void **)&processInfo)) {
    enqueuePQ(priorityQueue, processInfo,
              -processTable[processInfo->id]->priority);
  }
  free(processInfo);

  if (priorityQueue->head != NULL) {
    PriorityNode *process = priorityQueue->head->next;
    while (process != NULL) {
      int id = ((ProcessInfo *)(process->data))->id;
      processTable[id]->waitingTime += 1;
      process = process->next;
    }
  } else {
    return;
  }

  if (runningProcess == NULL) {
    if (!peekPQ(priorityQueue, (void **)&runningProcess)) {
      return;
    }
    startProcess(runningProcess);
  }

  PCB *pcb = processTable[runningProcess->id];

  if (!pcb->remainingTime) {
    removePQ(priorityQueue);
    removeProcess(runningProcess);
    runningProcess = NULL;
  } else {
    pcb->remainingTime -= 1;
    pcb->executionTime += 1;
  }
}

void srtn(){
}

void rr() {
  if (circularQueue == NULL) {
    circularQueue = newCircularQueue(sizeof(ProcessInfo));
  }

  ProcessInfo *processInfo = NULL;
  while (popFront(arrived, (void **)&processInfo)) {
    enqueueCQ(circularQueue, processInfo);
  }

  if (circularQueue->head != NULL) {
    Node *process = circularQueue->head->next;
    for (int i = 0; i < circularQueue->length - 1; ++i) {
      int id = ((ProcessInfo *)(process->data))->id;
      processTable[id]->waitingTime += 1;
      process = process->next;
    }
  } else {
    return;
  }

  if (runningProcess == NULL) {
    if (!peekCQ(circularQueue, (void **)&runningProcess)) {
      return;
    }
    resumeProcess(runningProcess);
    int tik = getClk();
    while (tik + QUANTA < getClk()) {
      usleep(DELAY_TIME);
    }
    PCB *pcb = processTable[runningProcess->id];

    if (!pcb->remainingTime) {
      removeCQ(circularQueue);
      removeProcess(runningProcess);
      runningProcess = NULL;
    } 
    else {
    pcb->remainingTime -= QUANTA;
    pcb->executionTime += QUANTA;
    if (pcb->remainingTime < 0)
      pcb->remainingTime = 0;
  }
    if (circularQueue->head->next && runningProcess != NULL) {
      stopProcess(runningProcess);
      runningProcess = (ProcessInfo*)(circularQueue->head->next->data);
      printf("Test %d\n",runningProcess->id);
    }
  }
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
    if (processTable != NULL) {
      free(processTable);
    }
    semctl(bufsemid, 0, IPC_RMID);
    shmdt(bufferaddr);
    destroyClk(false);
  }
  exit(0);
}
