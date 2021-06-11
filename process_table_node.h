#ifndef __PROCESS_TABLE_NODE_H
#define __PROCESS_TABLE_NODE_H
#include "headers.h"

/**
 * @brief  Struct used to represent a node in the process table .
 */
typedef struct process_table_node {
  int id;                               //process id
  struct PCB *pcb;                      // pointer to the block 
  struct PCB *next;    
} process_table_node ;

#endif
