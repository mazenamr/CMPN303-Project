#include "circular_queue.h"
#include "deque.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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

const int DELAY_TIME = 200000;

typedef enum SCHEDULING_ALGORITHM {
  NONE,
  FCFS,
  SJF,
  HPF,
  SRTN,
  RR
} SCHEDULING_ALGORITHM;

/**
 * @brief  Struct used to represent a process to be scheduled.
 */
typedef struct Process {
  int id;
  int arrival;
  int runtime;
  int priority;
} Process;

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

void printHelp() {
    printf("Usage: process_generator.out input.txt [scheduling algorithm]\n");
    printf("\nScheduling algorithms available:\n");
    printf("\t1. First Come First Serve (FCFS)\n");
    printf("\t2. Shortest Job First (SJF)\n");
    printf("\t3. Preemptive Highest Priority First (HPF)\n");
    printf("\t4. Shortest Remaining Time Next (SRTN)\n");
    printf("\t5. Round Robin (RR)\n");
}

