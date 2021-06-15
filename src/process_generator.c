#include "headers.h"

void clearResources(int);

static inline void setupIPC();
static inline void getInput(char*);

int shmid;
int bufsemid;
int *bufferaddr;

Deque *processes = NULL;

int main(int argc, char *argv[]) {
  pid_t pid;

  processes = newDeque(sizeof(Process));

  setupIPC();

  int *messageCount = (int *)bufferaddr;
  Process *buffer = (Process *)((void *)bufferaddr + sizeof(int));

  signal(SIGINT, clearResources);

  if (argc < 4) {
    printf("Too few arguments!\n");
    printHelp();
    exit(-1);
  }

  SCHEDULING_ALGORITHM sch = atoi(argv[2]);
  MEMORY_ALLOCATION_ALGORTHIM mem = atoi(argv[3]);

  if (sch < FCFS || sch > RR || mem < FIRSTFIT || mem > BUDDY) {
    printf("Invalid scheduling algorithm!\n");
    printHelp();
    exit(-1);
  }

  getInput(argv[1]);

  // start the clock process
  pid = fork();
  if (!pid) {
    execl("bin/clk.out", "clk.out", NULL);
  }

  // start the scheduler process
  pid = fork();
  if (!pid) {
    execl("bin/scheduler.out", "scheduler.out", argv[2], argv[3], NULL);
  }

  // initialize the clock counter
  initClk();

  // add each process to the buffer on reaching its arrival time
  Process *currentProcess = NULL;
  int endtime = getClk();
  while (processes->length) {
    down(bufsemid);
    int tick = getClk();
    while (peekFront(processes, (void **)&currentProcess)) {
      if (currentProcess->arrival <= getClk()) {
        removeFront(processes);
        while (*messageCount >= BUFFER_SIZE) {
          up(bufsemid);
          usleep(DELAY_TIME);
          down(bufsemid);
        }
        memcpy(buffer + (*messageCount)++, currentProcess, sizeof(Process));
        endtime = (currentProcess->arrival > endtime) ? currentProcess->arrival
                                                      : endtime;
        endtime += currentProcess->runtime;
        continue;
      }
      break;
    }
    up(bufsemid);
    while (tick == getClk()) {
      usleep(DELAY_TIME / 10);
    }
  }
  free(currentProcess);

  while (getClk() <= endtime) {
    usleep(DELAY_TIME);
  }

  // make sure everything ended properly
  sleep(5);
  down(bufsemid);

  clearResources(-1);
}

static inline void setupIPC() {
  semun s;
  s.val = 1;

  shmid = shmget(BUFKEY, sizeof(int) + sizeof(Process) * BUFFER_SIZE,
                 IPC_CREAT | 0644);
  if ((int)shmid == -1) {
    perror("Error in creating buffer!");
    exit(-1);
  }

  bufferaddr = (int *)shmat(shmid, (void *)0, 0);
  if ((long)bufferaddr == -1) {
    perror("Error in attaching the buffer in process generator!");
    exit(-1);
  }

  *(int *)bufferaddr = 0;

  bufsemid = semget(BUFSEMKEY, 1, 0644 | IPC_CREAT);
  if ((int)bufsemid == -1) {
    perror("Error in creating semaphore!");
    exit(-1);
  }

  if ((int)semctl(bufsemid, 0, SETVAL, s) == -1) {
    perror("Error in semctl!");
    exit(-1);
  }
}

static inline void getInput(char *file) {
  FILE *inputFile = fopen(file, "r");

  if (inputFile == NULL) {
    printf("Couldn't open input file %s!\n", file);
    exit(-1);
  }

  // read the input file line by line and create a process
  // for each line that doesn't start with #
  char line[MAX_LINE_SIZE];
  int lineNumber = 1;
  while (fscanf(inputFile, " %[^\n]s", line) != EOF) {
    Process process;
    if (line[0] != '#') {
      if (sscanf(line, "%d\t%d\t%d\t%d\t%d", &(process.id), &(process.arrival),
                 &(process.runtime), &(process.priority), &(process.memsize)) < 5) {
        printf("Error in input file line %d!\n", lineNumber);
        exit(-1);
      }
      pushBack(processes, &process);
    }
    ++lineNumber;
  }

  fclose(inputFile);
}

void clearResources(int signum) {
  // clear resources
  // but only if they weren't already cleared
  static bool ended = false;
  if (!ended) {
    ended = true;
    if (processes != NULL) {
      deleteDeque(processes);
    }
    semctl(bufsemid, 0, IPC_RMID);
    shmdt(bufferaddr);
    shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
    destroyClk(true);
  }
  exit(0);
}
