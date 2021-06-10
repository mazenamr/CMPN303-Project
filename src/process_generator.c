#include "headers.h"

void clearResources(int);
Deque *processes;
bool ended = false;

int main(int argc, char *argv[]) {
  pid_t pid;

  signal(SIGINT, clearResources);

  if (argc < 2) {
    printf("Too few arguments!\n");
    printHelp();
    exit(-1);
  }

  // TODO Initialization
  // 1. Read the input files.
  FILE *inputFile = fopen(argv[1], "r");

  if (inputFile == NULL) {
    printf("Couldn't open input file %s!\n", argv[1]);
    exit(-1);
  }

  processes = newDeque(sizeof(Process));

  char line[255];
  int lineNumber = 1;
  while (fscanf(inputFile, " %[^\n]s", line) != EOF) {
    Process process;
    if (line[0] != '#') {
      if (sscanf(line, "%d\t%d\t%d\t%d",
            &(process.id),
            &(process.arrival),
            &(process.runtime),
            &(process.priority)) < 4) {
        printf("Error in input file line %d!\n", lineNumber);
        exit(-1);
      }
      pushBack(processes, &process);
    }
    ++lineNumber;
  }

  Node *currentProcess = processes->head;
  for (int i = 0; i < processes->length; ++i) {
    printf("%d\t%d\t%d\t%d\n",
        ((Process*)(currentProcess->data))->id,
        ((Process*)(currentProcess->data))->arrival,
        ((Process*)(currentProcess->data))->runtime,
        ((Process*)(currentProcess->data))->priority);
    currentProcess = currentProcess->next;
  }

  fclose(inputFile);

  // 2. Read the chosen scheduling algorithm and its parameters, if there are
  // any from the argument list.
  SCHEDULING_ALGORITHM sch = NONE;
  if (argc > 2) {
    sch = atoi(argv[2]);
  }

  // 3. Initiate and create the scheduler and clock processes.
  // start the scheduler
  pid = fork();
  if (!pid) {
    execl("bin/scheduler.out", "scheduler.out", NULL);
  }

  // start the clock
  pid = fork();
  if (!pid) {
    execl("bin/clk.out", "clk.out", NULL);
  }

  // 4. Use this function after creating the clock process to initialize clock.
  initClk();
  // To get time use this function.
  int x = getClk();
  printf("Current Time is %d\n", x);

  // TODO Generation Main Loop
  // 5. Create a data structure for processes and provide it with its
  // parameters.
  // 6. Send the information to the scheduler at the appropriate time.
  // 7. Clear clock resources
  ended = true;
  deleteDeque(processes);
  destroyClk(true);
}

void clearResources(int signum) {
  // TODO Clears all resources in case of interruption
  if (!ended) {
    ended = true;
    destroyClk(true);
    deleteDeque(processes);
    exit(0);
  }
}
