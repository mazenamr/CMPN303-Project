#include "headers.h"

void clearResources(int);

static inline void setupIPC();
static inline void getInput(char*);

Deque *processes;
int shmid;
int semid;
int *shmaddr;

int main(int argc, char *argv[]) {
  pid_t pid;

  processes = newDeque(sizeof(Process));

  setupIPC();

  signal(SIGINT, clearResources);

  if (argc < 3) {
    printf("Too few arguments!\n");
    printHelp();
    exit(-1);
  }

  if (strlen(argv[2]) > 1) {
    printf("Invalid scheduling algorithm!\n");
    printHelp();
    exit(-1);
  }

  if (!isdigit(argv[2][0])) {
    printf("Invalid scheduling algorithm!\n");
    printHelp();
    exit(-1);
  }

  if (atoi(argv[2]) < FCFS || atoi(argv[2]) > RR) {
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
    execl("bin/scheduler.out", "scheduler.out", argv[2], NULL);
  }

  // initialize the clock counter
  initClk();

  // print the info of each process on reaching its arrival time
  Process *currentProcess = NULL;
  while (popFront(processes, (void *)&currentProcess)) {
    while (currentProcess->arrival > getClk()) {
      usleep(DELAY_TIME);
    }
    printf("%d\t%d\t%d\t%d\t%d\n", currentProcess->id, currentProcess->arrival,
           getClk(), currentProcess->runtime, currentProcess->priority);
    // TODO: setup the shared memory and semaphores
    // TODO: send the process to the scheduler
  }
  free(currentProcess);

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

  shmaddr = (int *)shmat(shmid, (void *)0, 0);
  if ((long)shmaddr == -1) {
    perror("Error in attaching the buffer in process generator!");
    exit(-1);
  }

  semid = semget(SEMKEY, 1, 0644 | IPC_CREAT);
  if ((int)semid == -1) {
    perror("Error in creating semaphore!");
    exit(-1);
  }

  if ((int)semctl(semid, 0, SETVAL, s) == -1) {
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
      if (sscanf(line, "%d\t%d\t%d\t%d", &(process.id), &(process.arrival),
                 &(process.runtime), &(process.priority)) < 4) {
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
    deleteDeque(processes);
    destroyClk(true);
  }
  exit(0);
}
