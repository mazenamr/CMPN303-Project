#include "headers.h"

static inline void setupIPC();
static inline void loadBuffer(bool);

void addProcess(Process*);
bool tryAllocate(int);
ProcessInfo startProcess(int);
void contProcess(ProcessInfo*);
void resumeProcess(ProcessInfo*);
void stopProcess(ProcessInfo*);
void removeProcess(ProcessInfo*);

void printMemory();
bool allocate(int, int, int);
bool deallocate(int, int);

void allocateBM(int, int);
void deallocateBM(int, int);
bool checkAllocateBM(int, int);

bool fcfs();
bool sjf();
bool hpf();
bool srtn();
bool rr();

int firstFit(PCB*);
int nextFit(PCB*);
int bestFit(PCB*);
int buddy(PCB*);

int firstFitBM(PCB*);
int nextFitBM(PCB*);

void clearResources(int);

int tick;

int shmid;
int bufsemid;
int procsemid;
int *bufferaddr;

int *messageCount;
Process *buffer;

bool bitMap[MEMORY_SIZE];
CircularQueue *memory = NULL;
Node *memoryHead = NULL;
Node *memoryLast = NULL;

Deque *arrived = NULL;
Deque *waiting = NULL;

SCHEDULING_ALGORITHM sch;
MEMORY_ALLOCATION_ALGORTHIM mem;

Deque *deque = NULL;
PriorityQueue *priorityQueue = NULL;
CircularQueue *circularQueue = NULL;

ProcessInfo *runningProcess = NULL;
PCB **processTable = NULL;
int processTableSize = PROCESS_TABLE_SIZE;

int totalCount = 0;
float totalWTA = 0;
int totalWait = 0;
int utilization = 0;
int lastAllocated = 0;

int main(int argc, char *argv[]) {
  setupIPC();

  messageCount = (int *)bufferaddr;
  buffer = (Process *)((void *)bufferaddr + sizeof(int));

  processTable = malloc(processTableSize * sizeof(PCB *));

  MemoryNode *memoryNode = malloc(sizeof(MemoryNode));
  memoryNode->start = 0;
  memoryNode->size = MEMORY_SIZE;
  memoryNode->process = 0;

  memory = newCircularQueue(sizeof(MemoryNode));
  enqueueCQ(memory, (void *)memoryNode);
  memoryHead = memory->head;
  free(memoryNode);

  arrived = newDeque(sizeof(PCB));
  waiting = newDeque(sizeof(int));

  signal(SIGINT, clearResources);

  initClk();

  if (argc < 3) {
    printf("No scheduling algorithm or memory allocation algirthim are provided!\n");
    printMemoryAllocationAlgorthims();
    printSchedulingAlgorithms();
    exit(-1);
  }

  sch = atoi(argv[1]);
  mem  = atoi(argv[2]);

  if (sch < FCFS || sch > RR ) {
    printf("Invalid scheduling algorithm!\n");
    printSchedulingAlgorithms();
    exit(-1);
  }

  if (mem < FIRSTFIT || mem > BUDDY ) {
    printf("Invalid Memory allocation algorithm!\n");
    printMemoryAllocationAlgorthims();
    exit(-1);
  }

  printf("#At\ttime\tx\tprocess\ty\tstate\t"
         "\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");
  FILE *pFile = fopen("scheduler.log", "w");
  fprintf(pFile, "#At\ttime\tx\tprocess\ty\tstate\t"
                 "\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");
  fclose(pFile);

  printf("#At\ttime\tx\tallocated\ty\tbytes\t"
         "for\tprocess\tz\tfrom\ti\tto\tj\n");
  pFile = fopen("memory.log", "w");
  fprintf(pFile, "#At\ttime\tx\tallocated\ty\tbytes\t"
                 "for\tprocess\tz\tfrom\ti\tto\tj\n");
  fclose(pFile);

  bool ran = false;
  while (true) {
    // ensure the process scheduler sent the new processes
    usleep(DELAY_TIME);
    loadBuffer(ran);
    tick = getClk();

    if (!ran) {
      switch (sch) {
      case FCFS:
        ran = fcfs();
        break;
      case SJF:
        ran = sjf();
        break;
      case HPF:
        ran = hpf();
        break;
      case SRTN:
        ran = srtn();
        break;
      case RR:
        ran = rr();
        break;
      default:
        printf("Invalid scheduling algorithm!\n");
        printSchedulingAlgorithms();
        exit(-1);
      }
      if (ran) {
        utilization += 1;
      }

      int *id = NULL;
      for (int i = 0; i < waiting->length; ++i) {
        popFront(waiting, (void **)&id);

        bool allocated = tryAllocate(*id);

        if (allocated) {
          ProcessInfo newProcess = startProcess(*id);
          pushBack(arrived, &newProcess);
        } else {
          pushBack(waiting, (void *)id);
        }
      }
      free(id);

      Node *node = waiting->head;
      for (int i = 0; i < waiting->length; ++i) {
        processTable[*((int *)(node->data))]->wait += 1;
        node = node->next;
      }
    }

    int *id = NULL;
    free(id);

    while (true) {
      down(bufsemid);
      if (*messageCount) {
        up(bufsemid);
        break;
      }
      usleep(DELAY_TIME);
      up(bufsemid);
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

static inline void loadBuffer(bool ran) {
  down(bufsemid);
  for (int i = 0; i < *messageCount; ++i) {
    Process *currentProcess = buffer + i;
    // if the new process id exceeds the size of the
    // process table then we need to increase the size
    // of the process table until it can fit
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
    addProcess(currentProcess);
    int id = currentProcess->id;

    bool allocated = tryAllocate(id);

    if (ran && allocated) {
      processTable[id]->wait += 1;
    }

    if (allocated) {
      ProcessInfo newProcess = startProcess(id);
      pushBack(arrived, &newProcess);
    } else {
      pushBack(waiting, &id);
    }
  }
  up(bufsemid);
  *messageCount = 0;
}

bool tryAllocate(int id) {
    PCB *pcb = processTable[id];
    int allocated = -1;
    switch (mem) {
    case FIRSTFIT:
      allocated = firstFit(pcb);
      break;
    case NEXTFIT:
      allocated = nextFit(pcb);
      break;
    case BESTFIT:
      allocated = bestFit(pcb);
      break;
    case BUDDY:
      allocated = buddy(pcb);
      break;
    default:
      printf("Invalid memory allocation algorithm!\n");
      printMemoryAllocationAlgorthims();
      exit(-1);
    }
    pcb->memstart = allocated;
    return (allocated != -1);
}

void addProcess(Process *process) {
  processTable[process->id] = malloc(sizeof(PCB));
  PCB *pcb = processTable[process->id];
  pcb->id = process->id;
  pcb->arrival = process->arrival;
  pcb->runtime = process->runtime;
  pcb->priority = process->priority;
  pcb->starttime = -1;
  pcb->remain = process->runtime;
  pcb->execution = 0;
  pcb->wait = 0;
  pcb->state = WAITING;
  pcb->memsize = process->memsize;
  pcb->memstart = -1;
}

ProcessInfo startProcess(int id) {
  PCB *pcb = processTable[id];
  char runtime[8];
  sprintf(runtime, "%d", pcb->runtime);
  pid_t pid = fork();
  if (!pid) {
    execl("process.out", "process.out", runtime, NULL);
  }
  // we need to make sure the process has
  // properly started before we stop it
  waitzero(procsemid);
  kill(pid, SIGSTOP);
  up(procsemid);
  ProcessInfo newProcess;
  newProcess.id = pcb->id;
  newProcess.pid = pid;
  return newProcess;
}

void contProcess(ProcessInfo *process) {
  kill(process->pid, SIGCONT);
  PCB *pcb = processTable[process->id];
  pcb->state = RUNNING;
  char *started = "resumed";
  if (pcb->starttime < 0) {
    pcb->starttime = tick;
    started = "started";
  }
  printf("At\ttime\t%d\tprocess\t%d\t%s\t"
         "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
         tick, process->id, started, pcb->arrival, pcb->runtime, pcb->remain,
         pcb->wait);
  FILE *pFile = fopen("scheduler.log", "a");
  fprintf(pFile,
          "At\ttime\t%d\tprocess\t%d\t%s\t"
          "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
          tick, process->id, started, pcb->arrival, pcb->runtime, pcb->remain,
          pcb->wait);
  fclose(pFile);
}

void stopProcess(ProcessInfo *process) {
  kill(process->pid, SIGSTOP);
  PCB *pcb = processTable[process->id];
  pcb->state = WAITING;
  printf("At\ttime\t%d\tprocess\t%d\tstopped\t"
         "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
         tick, process->id, pcb->arrival, pcb->runtime, pcb->remain, pcb->wait);
  FILE *pFile = fopen("scheduler.log", "a");
  fprintf(pFile,
          "At\ttime\t%d\tprocess\t%d\tstopped\t"
          "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
          tick, process->id, pcb->arrival, pcb->runtime, pcb->remain,
          pcb->wait);
  fclose(pFile);
}

void removeProcess(ProcessInfo *process) {
  PCB *pcb = processTable[process->id];
  float WTA = (tick - pcb->arrival) / (float)(pcb->runtime);
  totalCount += 1;
  totalWTA += WTA;
  totalWait += pcb->wait;
  printf("At\ttime\t%d\tprocess\t%d\tfinished"
         "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d"
         "\tTA\t%d\tWTA\t%0.2f\n",
         tick, process->id, pcb->arrival, pcb->runtime, pcb->remain, pcb->wait,
         tick - pcb->arrival, WTA);
  FILE *pFile = fopen("scheduler.log", "a");
  fprintf(pFile,
          "At\ttime\t%d\tprocess\t%d\tfinished"
          "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d"
          "\tTA\t%d\tWTA\t%0.2f\n",
          tick, process->id, pcb->arrival, pcb->runtime, pcb->remain, pcb->wait,
          tick - pcb->arrival, WTA);
  fclose(pFile);
  pFile = fopen("scheduler.perf", "w");
  fprintf(pFile, "CPU utilization = %0.2f%%\n", 100 * utilization / (float)(tick - 1));
  fprintf(pFile, "Avg WTA = %0.2f\n", totalWTA / (float)totalCount);
  fprintf(pFile, "Avg Waiting = %0.2f\n", totalWait / (float)totalCount);
  fclose(pFile);
  deallocate(pcb->memsize, pcb->id);
  free(pcb);
}

void printMemory() {
  Node *node = memoryHead;
  for (int i = 0; i < memory->length; ++i) {
    MemoryNode *memoryNode = (MemoryNode *)node->data;
    printf("%d -> %d = %d : %d\n", memoryNode->start,
           memoryNode->start + memoryNode->size - 1, memoryNode->size,
           memoryNode->process);
    node = node->next;
  }
  printf("\n");
}

bool allocate(int start, int size, int process) {
  MemoryNode *memoryNode = NULL;
  int end = start + size - 1;
  for (int i = 0; i < memory->length; ++i) {
    moveNext(memory, (void **)&memoryNode);
    if (memoryNode->start != start) {
      continue;
    }
    if (memoryNode->size < size) {
      continue;
    }
    removeCQ(memory);
    MemoryNode *newNode = malloc(sizeof(MemoryNode));
    newNode->start = start;
    newNode->size = size;
    newNode->process = process;
    enqueueCQ(memory, (void *)newNode);
    memoryLast = memory->head->prev;
    if (!start) {
      memoryHead = memory->head->prev;
    }

    printf("#At\ttime\t%d\tallocated\t%d\tbytes\t"
          "for\tprocess\t%d\tfrom\t%d\tto\t%d\n",
          tick, size, process, start, start + size - 1);
    FILE *pFile = fopen("memory.log", "a");
    fprintf(pFile,
          "#At\ttime\t%d\tallocated\t%d\tbytes\t"
          "for\tprocess\t%d\tfrom\t%d\tto\t%d\n",
          tick, size, process, start, start + size - 1);
    fclose(pFile);

    if (memoryNode->size > size) {
      newNode->start = start + size;
      newNode->size = memoryNode->size - size;
      newNode->process = 0;
      enqueueCQ(memory, (void *)newNode);
    }
    free(newNode);
    free(memoryNode);
    return true;
  }
  free(memoryNode);
  return false;
}

bool deallocate(int start, int process) {
  MemoryNode *memoryNode = NULL;
  for (int i = 0; i < memory->length; ++i) {
    moveNext(memory, (void **)&memoryNode);
    if (memoryNode->start != start) {
      continue;
    }
    removeCQ(memory);
    MemoryNode *newNode = malloc(sizeof(MemoryNode));
    newNode->start = start;
    newNode->size = memoryNode->size;
    newNode->process = 0;
    printf("#At\ttime\t%d\tfreed\t%d\tbytes\t"
          "for\tprocess\t%d\tfrom\t%d\tto\t%d\n",
          tick, newNode->size, process, start, start + newNode->size - 1);
    FILE *pFile = fopen("memory.log", "a");
    fprintf(pFile,
          "#At\ttime\t%d\tfreed\t%d\tbytes\t"
          "for\tprocess\t%d\tfrom\t%d\tto\t%d\n",
          tick, newNode->size, process, start, start + newNode->size - 1);
    fclose(pFile);
    if (peekCQ(memory, (void **)&memoryNode)) {
      if (memoryNode->start > newNode->start) {
        if (!memoryNode->process) {
          removeCQ(memory);
          newNode->size += memoryNode->size;
        }
      }
    }
    if (movePrev(memory, (void **)&memoryNode)) {
      if (memoryNode->start < newNode->start) {
        if (!memoryNode->process) {
          removeCQ(memory);
          newNode->start = memoryNode->start;
          newNode->size += memoryNode->size;
        }
      }
    }
    enqueueCQ(memory, (void *)newNode);
    if (!newNode->start) {
      memoryHead = memory->head->prev;
    }
    free(newNode);
    free(memoryNode);
    return true;
  }
  free(memoryNode);
  return false;
}

bool fcfs() {
  PCB *pcb;
  ProcessInfo *processInfo;

  // initialize deque of project info if it's not initialized
  if (deque == NULL) {
    deque = newDeque(sizeof(ProcessInfo));
  }

  // load the newly arrived processes into the deque
  processInfo = NULL;
  while (popFront(arrived, (void **)&processInfo)) {
    pushBack(deque, processInfo);
  }
  free(processInfo);

  // if the deque is empty, there is nothing to do
  if (deque->head == NULL) {
    return false;
  }

  // if the running process has finished
  // then we need to remove it
  if (runningProcess != NULL) {
    pcb = processTable[runningProcess->id];
    if (pcb->remain <= 0) {
      removeFront(deque);
      removeProcess(runningProcess);
      free(runningProcess);
      runningProcess = NULL;
    }
  }

  // if we don't have a running process then we
  // should start the next process in the deque
  if (runningProcess == NULL) {
    // if we don't have a running process
    // and the deque is empty, then there
    // is nothing to do
    if (!peekFront(deque, (void **)&runningProcess)) {
      return false;
    }
    contProcess(runningProcess);
  }

  // the head of the deque should be
  // the currently running process
  // so we will increase the wait time
  // of all the processes after the head
  Node *process = deque->head->next;
  while (process != NULL) {
    int id = ((ProcessInfo *)(process->data))->id;
    processTable[id]->wait += 1;
    process = process->next;
  }

  // update the pcb of the running process
  pcb = processTable[runningProcess->id];
  pcb->remain -= 1;
  pcb->execution += 1;

  return true;
}

bool sjf() {
  PCB *pcb;
  ProcessInfo *processInfo;

  // initialize priority queue of project info if it's not initialized
  if (priorityQueue == NULL) {
    priorityQueue = newPriorityQueue(sizeof(ProcessInfo));
  }

  // if the priority queue is empty, there is nothing to do
  if (priorityQueue->head == NULL) {
    if (arrived->head == NULL) {
      return false;
    }
  }

  // if the running process has finished
  // then we need to remove it
  if (runningProcess != NULL) {
    pcb = processTable[runningProcess->id];
    if (pcb->remain <= 0) {
      removePQ(priorityQueue);
      removeProcess(runningProcess);
      free(runningProcess);
      runningProcess = NULL;
    }
  }

  // load the newly arrived processes into the priority queue
  processInfo = NULL;
  while (popFront(arrived, (void **)&processInfo)) {
    pcb = processTable[processInfo->id];
    enqueuePQ(priorityQueue, processInfo, -1 * (pcb->runtime), true);
  }
  free(processInfo);

  // if we don't have a running process then we
  // should start the next process in the priority queue
  if (runningProcess == NULL) {
    // if we don't have a running process
    // and the priority queue is empty, then there
    // is nothing to do
    if (!peekPQ(priorityQueue, (void **)&runningProcess)) {
      return false;
    }
    contProcess(runningProcess);
  }

  // the head of the priority queue should be
  // the currently running process
  // so we will increase the wait time
  // of all the processes after the head
  PriorityNode *process = priorityQueue->head->next;
  while (process != NULL) {
    int id = ((ProcessInfo *)(process->data))->id;
    processTable[id]->wait += 1;
    process = process->next;
  }

  // we also need to update the wait time
  // of the processes that arrived
  Node *arrivedProcess = arrived->head;
  while (arrivedProcess != NULL) {
    int id = ((ProcessInfo *)(arrivedProcess->data))->id;
    processTable[id]->wait += 1;
    arrivedProcess = arrivedProcess->next;
  }

  // update the pcb of the running process
  pcb = processTable[runningProcess->id];
  pcb->remain -= 1;
  pcb->execution += 1;

  return true;
}

bool hpf() {
  PCB *pcb;
  ProcessInfo *processInfo;

  // initialize priority queue of project info if it's not initialized
  if (priorityQueue == NULL) {
    priorityQueue = newPriorityQueue(sizeof(ProcessInfo));
  }

  // if the priority queue is empty, there is nothing to do
  if (priorityQueue->head == NULL) {
    if (arrived->head == NULL) {
      return false;
    }
  }

  // if the running process has finished
  // then we need to remove it
  if (runningProcess != NULL) {
    pcb = processTable[runningProcess->id];
    if (pcb->remain <= 0) {
      removePQ(priorityQueue);
      removeProcess(runningProcess);
      free(runningProcess);
      runningProcess = NULL;
    }
  }

  // load the newly arrived processes into the priority queue
  processInfo = NULL;
  while (popFront(arrived, (void **)&processInfo)) {
    pcb = processTable[processInfo->id];
    enqueuePQ(priorityQueue, processInfo, -1 * (pcb->priority), false);
  }
  free(processInfo);

  // if we don't have a running process then we
  // should start the next process in the priority queue
  if (runningProcess == NULL) {
    // if we don't have a running process
    // and the priority queue is empty, then there
    // is nothing to do
    if (!peekPQ(priorityQueue, (void **)&runningProcess)) {
      return false;
    }
    contProcess(runningProcess);
  } else {
    // we always switch the running process to the first
    // process in the priority queue
    processInfo = NULL;
    peekPQ(priorityQueue, (void **)&processInfo);
    if (processInfo->id != runningProcess->id) {
      stopProcess(runningProcess);
      free(runningProcess);
      runningProcess = processInfo;
      contProcess(runningProcess);
    } else {
      free(processInfo);
    }
  }

  // the head of the priority queue should be
  // the currently running process
  // so we will increase the wait time
  // of all the processes after the head
  PriorityNode *process = priorityQueue->head->next;
  while (process != NULL) {
    int id = ((ProcessInfo *)(process->data))->id;
    processTable[id]->wait += 1;
    process = process->next;
  }

  // update the pcb of the running process
  pcb = processTable[runningProcess->id];
  pcb->remain -= 1;
  pcb->execution += 1;

  return true;
}

bool srtn(){
  PCB *pcb;
  ProcessInfo *processInfo;

  // initialize priority queue of project info if it's not initialized
  if (priorityQueue == NULL) {
    priorityQueue = newPriorityQueue(sizeof(ProcessInfo));
  }

  // if the priority queue is empty, there is nothing to do
  if (priorityQueue->head == NULL) {
    if (arrived->head == NULL) {
      return false;
    }
  }

  // if the running process has finished
  // then we need to remove it
  if (runningProcess != NULL) {
    pcb = processTable[runningProcess->id];
    if (pcb->remain <= 0) {
      removePQ(priorityQueue);
      removeProcess(runningProcess);
      free(runningProcess);
      runningProcess = NULL;
    }
  }

  // we need to change the priority of the running process
  // to the remaining time instead of the total runtime
  if (runningProcess != NULL) {
    pcb = processTable[runningProcess->id];
    priorityQueue->head->priority = -1 * (pcb->remain);
  }

  // load the newly arrived processes into the priority queue
  processInfo = NULL;
  while (popFront(arrived, (void **)&processInfo)) {
    pcb = processTable[processInfo->id];
    enqueuePQ(priorityQueue, processInfo, -1 * (pcb->runtime), false);
  }
  free(processInfo);

  // if we don't have a running process then we
  // should start the next process in the priority queue
  if (runningProcess == NULL) {
    // if we don't have a running process
    // and the priority queue is empty, then there
    // is nothing to do
    if (!peekPQ(priorityQueue, (void **)&runningProcess)) {
      return false;
    }
    contProcess(runningProcess);
  } else {
    // we always switch the running process to the first
    // process in the priority queue
    processInfo = NULL;
    peekPQ(priorityQueue, (void **)&processInfo);
    if (processInfo->id != runningProcess->id) {
      stopProcess(runningProcess);
      free(runningProcess);
      runningProcess = processInfo;
      contProcess(runningProcess);
    } else {
      free(processInfo);
    }
  }

  // the head of the priority queue should be
  // the currently running process
  // so we will increase the wait time
  // of all the processes after the head
  PriorityNode *process = priorityQueue->head->next;
  while (process != NULL) {
    int id = ((ProcessInfo *)(process->data))->id;
    processTable[id]->wait += 1;
    process = process->next;
  }

  // update the pcb of the running process
  pcb = processTable[runningProcess->id];
  pcb->remain -= 1;
  pcb->execution += 1;

  return true;
}

bool rr() {
  PCB *pcb;
  ProcessInfo *processInfo;
  static int q = 0;

  // initialize circular queue of project info if it's not initialized
  if (circularQueue == NULL) {
    circularQueue = newCircularQueue(sizeof(ProcessInfo));
  }

  // if the circular queue is empty, there is nothing to do
  if (circularQueue->head == NULL) {
    if (arrived->head == NULL) {
      return false;
    }
  }

  // if the running process has finished
  // then we need to remove it
  if (runningProcess != NULL) {
    pcb = processTable[runningProcess->id];
    if (pcb->remain <= 0) {
      removeCQ(circularQueue);
      removeProcess(runningProcess);
      free(runningProcess);
      runningProcess = NULL;
      q = 0;
    }
  }

  // load the newly arrived processes into the circular queue
  processInfo = NULL;
  while (popFront(arrived, (void **)&processInfo)) {
    pcb = processTable[processInfo->id];
    enqueueCQ(circularQueue, processInfo);
  }
  free(processInfo);

  if (!q) {
    if (runningProcess == NULL) {
      peekCQ(circularQueue, (void **)&runningProcess);
      contProcess(runningProcess);
    } else {
      processInfo = NULL;
      moveNext(circularQueue, (void **)&processInfo);
      if (processInfo->id != runningProcess->id) {
        stopProcess(runningProcess);
        free(runningProcess);
        runningProcess = processInfo;
        contProcess(runningProcess);
      } else {
        free(processInfo);
      }
    }
  }

  q = (q + 1) % QUANTA;

  // the head of the circular queue should be
  // the currently running process
  // so we will increase the wait time
  // of all the processes after the head
  Node *process = circularQueue->head->next;
  for (int i = 0; i < circularQueue->length - 1; ++i) {
    int id = ((ProcessInfo *)(process->data))->id;
    processTable[id]->wait += 1;
    process = process->next;
  }

  // update the pcb of the running process
  pcb = processTable[runningProcess->id];
  pcb->remain -= 1;
  pcb->execution += 1;

  return true;
}

int firstFitBM(PCB* process){
  for (int i = 0; i < MEMORY_SIZE; i++) {
    if (checkAllocateBM(i, process->memsize) != -1) {
      allocateBM(i, process->memsize);
      processTable[process->id]->memstart = i;
      return i;
    }
  }
  return -1;
}


int nextFitBM(PCB* process){
  for (int i = lastAllocated; i < MEMORY_SIZE; i++) {
    if (checkAllocateBM(i, process->memsize)) {
      processTable[process->memsize]->memstart = i;
      allocateBM(i, process->memsize);
      lastAllocated = i + process->memsize;
      return i;
    }
  }
  return -1;
}

bool checkAllocateBM(int start, int size) {
  for (int i = 0; i < size; i++) {
    if (bitMap[(start + i) % MEMORY_SIZE]) {
      return false;
    }
  }
  return true;
}

void allocateBM(int start, int size) {
  for (int i = 0; i < size; i++) {
    bitMap[(start + i) % MEMORY_SIZE] = true;
  }
}

void deallocateBM(int start, int size) {
  for (int i = 0; i < size; i++) {
    bitMap[(start + i) % MEMORY_SIZE] = false;
  }
}

int firstFit(PCB* process) {
  Node *node = memoryHead;
  for (int i = 0; i < memory->length; ++i) {
    MemoryNode *memoryNode = (MemoryNode *)node->data;
    if (memoryNode->size >= process->memsize && memoryNode->process == 0) {
      if(allocate(memoryNode->start, process->memsize, process->id)) {
        return memoryNode->start;
      }
    }
    node = node->next;
  }
  return -1;
}

int nextFit(PCB* process) {
  Node *node = memoryLast;
  for (int i = 0; i < memory->length; ++i) {
    MemoryNode *memoryNode = (MemoryNode *)node->data;
    if (memoryNode->size >= process->memsize && memoryNode->process == 0) {
      if(allocate(memoryNode->start, process->memsize, process->id)) {
        return memoryNode->start;
      }
    }
    node = node->next;
  }
  return -1;
}

int bestFit(PCB* process){
  int minNeededSize = process->memsize;
  int currentMinSize = MEMORY_SIZE+1;

  Node *node = memoryHead;
  MemoryNode* fitNode;
  bool allocated = false;
  for (int i = 0; i < memory->length; ++i) {
    MemoryNode *memoryNode = (MemoryNode *)node->data;
    if (memoryNode->size >= process->memsize && memoryNode->process == 0) {
      if (currentMinSize >= memoryNode->size){
        currentMinSize = memoryNode->size;
        fitNode = memoryNode;
      }
    }
    node = node->next;
  }
  if (allocate(fitNode->start, process->memsize, process->id) ) {
    return fitNode->start;
  }

  return -1;
}


int buddy(PCB* process){
  return 1;
}

int ceilPowerOfTwo(int num) {
  num--;
  num |= num >> 1;
  num |= num >> 2;
  num |= num >> 4;
  num |= num >> 8;
  num |= num >> 16;
  return ++num;
}

void clearResources(int signum) {
  // clear resources
  // but only if they weren't already cleared
  static bool ended = false;
  if (!ended) {
    ended = true;
    if (memory != NULL) {
      deleteCircularQueue(memory);
    }
    if (arrived != NULL) {
      deleteDeque(arrived);
    }
    if (waiting != NULL) {
      deleteDeque(waiting);
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
