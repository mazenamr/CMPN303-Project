#include "headers.h"
#include <unistd.h>

static inline void setupIPC();
static inline void loadBuffer();

ProcessInfo addProcess(Process*);
void contProcess(ProcessInfo*);
void resumeProcess(ProcessInfo*);
void stopProcess(ProcessInfo*);
void removeProcess(ProcessInfo*);

bool fcfs();
bool sjf();
bool hpf();
bool srtn();
bool rr();

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
         "\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");
  while (true) {
    loadBuffer();
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
  pcb->startTime = -1;
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

void contProcess(ProcessInfo *process) {
  kill(process->pid, SIGCONT);
  PCB *pcb = processTable[process->id];
  if (pcb->startTime < 0) {
    pcb->startTime = tick;
  }
  printf("At\ttime\t%d\tprocess\t%d\tstarted\t"
         "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
         tick, process->id, pcb->arrival, pcb->runtime, pcb->remainingTime,
         pcb->waitingTime);
}

void stopProcess(ProcessInfo *process) {
  kill(process->pid, SIGSTOP);
  PCB *pcb = processTable[process->id];
  printf("At\ttime\t%d\tprocess\t%d\tstopped\t"
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
         "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
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
    processTable[id]->waitingTime += 1;
    process = process->next;
  }

  // update the pcb of the running process
  pcb = processTable[runningProcess->id];
  pcb->remainingTime -= 1;
  pcb->executionTime += 1;

  return true;
}

bool sjf() {
  PCB *pcb;
  ProcessInfo *processInfo;
  // initialize priority queue of project info if it's not initialized
  if (priorityQueue == NULL) {
    priorityQueue = newPriorityQueue(sizeof(ProcessInfo));
  }

  // here we only load the newly arrived processes when the
  // priority queue is empty because we need to make sure
  // that the head of the priority queue doesn't change
  // so that we can easily dequeue it
  if (priorityQueue->head == NULL) {
    // if the priority queue is empty and no new processes
    // have arrived, there is nothing to do
    if (arrived->head == NULL) {
      return false;
    }
    // load the newly arrived processes into the priority queue
    processInfo = NULL;
    while (popFront(arrived, (void **)&processInfo)) {
      pcb = processTable[processInfo->id];
      enqueuePQ(priorityQueue, processInfo, -1 * (pcb->runtime));
    }
    free(processInfo);
  }

  // if the running process has finished
  // then we need to remove it
  if (runningProcess != NULL) {
    pcb = processTable[runningProcess->id];
    if (!pcb->remainingTime) {
      removePQ(priorityQueue);
      removeProcess(runningProcess);
      free(runningProcess);
      runningProcess = NULL;
    }
  }

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
    processTable[id]->waitingTime += 1;
    process = process->next;
  }

  // update the pcb of the running process
  pcb = processTable[runningProcess->id];
  pcb->remainingTime -= 1;
  pcb->executionTime += 1;

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
    if (!pcb->remainingTime) {
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
    enqueuePQ(priorityQueue, processInfo, -1 * (pcb->priority));
  }

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
    peekPQ(priorityQueue, (void **)&processInfo);
    if (processInfo->id != runningProcess->id) {
      stopProcess(runningProcess);
      free(runningProcess);
      runningProcess = processInfo;
      contProcess(runningProcess);
    }
    else {
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
    processTable[id]->waitingTime += 1;
    process = process->next;
  }

  // update the pcb of the running process
  pcb = processTable[runningProcess->id];
  pcb->remainingTime -= 1;
  pcb->executionTime += 1;

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
    if (!pcb->remainingTime) {
      removePQ(priorityQueue);
      removeProcess(runningProcess);
      free(runningProcess);
      runningProcess = NULL;
    }
  }

  // we need to change the priorty of the running process
  // to the remaining time instead of the total runtime
  if (runningProcess != NULL) {
    pcb = processTable[runningProcess->id];
    priorityQueue->head->priority = -1 * (pcb->remainingTime);
  }

  // load the newly arrived processes into the priority queue
  processInfo = NULL;
  while (popFront(arrived, (void **)&processInfo)) {
    pcb = processTable[processInfo->id];
    enqueuePQ(priorityQueue, processInfo, -1 * (pcb->runtime));
  }

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
    peekPQ(priorityQueue, (void **)&processInfo);
    if (processInfo->id != runningProcess->id) {
      stopProcess(runningProcess);
      free(runningProcess);
      runningProcess = processInfo;
      contProcess(runningProcess);
    }
    else {
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
    processTable[id]->waitingTime += 1;
    process = process->next;
  }

  // update the pcb of the running process
  pcb = processTable[runningProcess->id];
  pcb->remainingTime -= 1;
  pcb->executionTime += 1;

  return true;
}

  // if the running process has finished
  // then we need to remove it
  if (runningProcess != NULL) {
    pcb = processTable[runningProcess->id];
    if (!pcb->remainingTime) {
      // removeElement(priorityQueue, runningProcess);
      removePQ(priorityQueue);
      removeProcess(runningProcess);
      free(runningProcess);
      runningProcess = NULL;
    }
    return true;
  }
  return false;
}

bool rr() {
  return true;
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
