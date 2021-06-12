#include "circular_queue.h"
#include "deque.h"
#include "priority_queue.h"
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SHKEY 300
#define BUFKEY 400
#define BUFSEMKEY 500
#define PROCSEMKEY 600

// 1,000,000 = 1 sec
#define CLOCK_TICK_DURATION 1000000
#define DELAY_TIME 100

#define BUFFER_SIZE 128
#define MAX_LINE_SIZE 256
#define PROCESS_TABLE_SIZE 4096

typedef enum SCHEDULING_ALGORITHM {
  NONE,
  FCFS,
  SJF,
  HPF,
  SRTN,
  RR
} SCHEDULING_ALGORITHM;

typedef enum PROCESS_STATE {
  READY,
  RUNNING,
  BLOCKED,
  FINISHED
} PROCESS_STATE ;

/**
 * @brief  Struct used to represent a process to be scheduled.
 */
typedef struct Process {
  int id;
  int arrival;
  int runtime;
  int priority;
} Process;

typedef struct ProcessInfo {
  int id;
  pid_t pid;
} ProcessInfo;

/**
 * @brief  Struct to represent the process control block
 *         which contains various info about a process.
 */
typedef struct PCB
{
  int id;
  int arrival;
  int runtime;
  int priority;
  int startTime;
  int remainingTime;
  int executionTime;
  int waitingTime;
  PROCESS_STATE state;
}PCB;

// semun used to modify semaphore settings
typedef union semun {
  int val;               /* Value for SETVAL */
  struct semid_ds *buf;  /* Buffer for IPC_STAT, IPC_SET */
  unsigned short *array; /* Array for GETALL, SETALL */
  struct seminfo *__buf; /* Buffer for IPC_INFO
                            (Linux-specific) */
} semun;

///==============================
// don't mess with this variable//
int *shmaddr; //
//===============================

int getClk() { return *shmaddr; }

/*
 * All processes call this function at the beginning to establish communication
 * between them and the clock module. Again, remember that the clock is only
 * emulation!
 */
void initClk() {
  int shmid = shmget(SHKEY, 4, 0444);
  while ((int)shmid == -1) {
    // Make sure that the clock exists
    printf("Wait! The clock not initialized yet!\n");
    sleep(1);
    shmid = shmget(SHKEY, 4, 0444);
  }
  shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

/*
 * All processes call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of
 * simulation. It terminates the whole system and releases resources.
 */
void destroyClk(bool terminateAll) {
  shmdt(shmaddr);
  if (terminateAll) {
    killpg(getpgrp(), SIGINT);
  }
}

void down(int sem) {
  struct sembuf p_op;

  p_op.sem_num = 0;
  p_op.sem_op = -1;
  p_op.sem_flg = !IPC_NOWAIT;

  if (semop(sem, &p_op, 1) == -1) {
    perror("Error in down()");
    exit(-1);
  }
}

void up(int sem) {
  struct sembuf p_op;

  p_op.sem_num = 0;
  p_op.sem_op = 1;
  p_op.sem_flg = !IPC_NOWAIT;

  if (semop(sem, &p_op, 1) == -1) {
    perror("Error in up()");
    exit(-1);
  }
}

void waitzero(int sem) {
  struct sembuf p_op;

  p_op.sem_num = 0;
  p_op.sem_op = 0;
  p_op.sem_flg = !IPC_NOWAIT;

  if (semop(sem, &p_op, 1) == -1) {
    perror("Error in waitzero()");
    exit(-1);
  }
}

bool trydown(int sem) {
  struct sembuf p_op;

  p_op.sem_num = 0;
  p_op.sem_op = -1;
  p_op.sem_flg = IPC_NOWAIT;

  return (semop(sem, &p_op, 1) != -1);
}

void printSchedulingAlgorithms() {
  printf("\nScheduling algorithms available:\n");
  printf("\t1. First Come First Serve (FCFS)\n");
  printf("\t2. Shortest Job First (SJF)\n");
  printf("\t3. Preemptive Highest Priority First (HPF)\n");
  printf("\t4. Shortest Remaining Time Next (SRTN)\n");
  printf("\t5. Round Robin (RR)\n");
}

void printHelp() {
  printf("Usage: process_generator.out [input file] [scheduling algorithm]\n");
  printSchedulingAlgorithms();
  printf("ex: process_generator.out input.txt 2\n");
}
