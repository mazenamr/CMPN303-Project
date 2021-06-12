#include "headers.h"
#include <unistd.h>

void clearResources(int);
static inline void setupIPC();
ProcessInfo addProcess(Process*);
void startProcess(ProcessInfo*);
void resumeProcess(ProcessInfo*);
void stopProcess(ProcessInfo*);
void removeProcess(ProcessInfo*);

void fcfs();
void sjf();
void hpf();
void srtn();
void rr();


int shmid;
int bufsemid;
int procsemid;
int *bufferaddr;

ProcessInfo *runningProcess = NULL;
Deque *arrived = NULL;
Deque *deque = NULL;
PriorityQueue *priorityQueue = NULL;
CircularQueue *circularQueue = NULL;

PCB **processTable = NULL;
int processTableSize = PROCESS_TABLE_SIZE;

int main(int argc, char *argv[]) {
  setupIPC();

  processTable = malloc(processTableSize * sizeof(PCB *));

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
      down(bufsemid);
      if (*messageCount) {
        up(bufsemid);
        break;
      }
      up(bufsemid);
      usleep(DELAY_TIME);
    }
  }

  clearResources(-1);
}

void fcfs() {
  if (deque == NULL) {
    deque = newDeque(sizeof(ProcessInfo));
  }

  ProcessInfo *processInfo = NULL;
  while (popFront(arrived, (void **)&processInfo)) {
    pushBack(deque, processInfo);
  }
  free(processInfo);

  if (deque->head != NULL) {
    Node *process = deque->head->next;
    while (process != NULL) {
      int id = ((ProcessInfo *)(process->data))->id;
      processTable[id]->waitingTime += 1;
      process = process->next;
    }
  } else {
    return;
  }

  if (runningProcess == NULL) {
    if (!peekFront(deque, (void **)&runningProcess)) {
      return;
    }
    startProcess(runningProcess);
  }

  PCB *pcb = processTable[runningProcess->id];

  if (!pcb->remainingTime) {
    removeFront(deque);
    removeProcess(runningProcess);
    runningProcess = NULL;
  } else {
    pcb->remainingTime -= 1;
    pcb->executionTime += 1;
  }
}

void sjf() {
  if (priorityQueue == NULL) {
    priorityQueue = newPriorityQueue(sizeof(ProcessInfo));
  }

  ProcessInfo *processInfo = NULL;
  while (popFront(arrived, (void **)&processInfo)) {
    enqueuePQ(priorityQueue, processInfo, -processTable[processInfo->id]->runtime);
  }
  free(processInfo);

  if (priorityQueue->head != NULL) {
    PriorityNode* process = priorityQueue->head->next;
    while (process!= NULL) {
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
    enqueuePQ(priorityQueue, processInfo, -processTable[processInfo->id]->priority);
  }
  free(processInfo);

  if (priorityQueue->head != NULL) {
    PriorityNode* process = priorityQueue->head->next;
    while (process!= NULL) {
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
<<<<<<< HEAD
  if (circularQueue == NULL) {
    circularQueue = newCircularQueue(sizeof(ProcessInfo));
  }

  ProcessInfo *processInfo = NULL;
  while (popFront(arrived, (void **)&processInfo)) {
    enqueueCQ(circularQueue, processInfo);
  }

  if (circularQueue->head != NULL) {
    Node* process = circularQueue->head->next;
    for (int i = 0; i < circularQueue->length-1; ++i) {
      int id = ((ProcessInfo *)(process->data))->id;
      processTable[id]->waitingTime += 1;
      process = process->next;
    }
  }
  else {
    return;
  }
  
  if (runningProcess == NULL) {
    if (!peekCQ(circularQueue, (void **)& runningProcess)) {
      return;
    }
    resumeProcess(runningProcess);
    int tik = getClk();
    while (tik + QUANTA < getClk()) {
      usleep(DELAY_TIME);
    }
    free(runningProcess);
    dequeueCQ(circularQueue, (void **)&runningProcess);
    if(peekCQ(circularQueue, (void **)&processInfo)) {
      stopProcess(runningProcess);
    }
    enqueueCQ(circularQueue, runningProcess);
  }

  PCB *pcb = processTable[runningProcess->id];

  if (!pcb->remainingTime) {
    removeCQ(circularQueue);
    removeProcess(runningProcess);
    runningProcess = NULL;
  } else {
    pcb->remainingTime -= QUANTA;
    pcb->executionTime += QUANTA;
    if (pcb->remainingTime < 0)
      pcb->remainingTime = 0;
  }
=======

>>>>>>> main
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
  pcb->startTime = getClk();
  printf("At\ttime\t%d\tprocess\t%d\tstarted\t"
         "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
         getClk(), process->id, pcb->arrival, pcb->runtime, pcb->remainingTime,
         pcb->waitingTime);
}

void resumeProcess(ProcessInfo *process) {
  kill(process->pid, SIGCONT);
  PCB *pcb = processTable[process->id];
  printf("At\ttime\t%d\tprocess\t%d\tresumed\t"
         "\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
         getClk(), process->id, pcb->arrival, pcb->runtime, pcb->remainingTime,
         pcb->waitingTime);
}

void stopProcess(ProcessInfo *process) {
  kill(process->pid, SIGSTOP);
  PCB *pcb = processTable[process->id];
  printf("At\ttime\t%d\tprocess\t%d\tresumed\t"
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
    if (processTable != NULL) {
      free(processTable);
    }
    semctl(bufsemid, 0, IPC_RMID);
    shmdt(bufferaddr);
    destroyClk(false);
  }
  exit(0);
}
