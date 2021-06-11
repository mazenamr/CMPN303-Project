#ifndef __PRIORITY_QUEUE_H
#define __PRIORITY_QUEUE_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief  Struct used to represent one element
 *         in a priority queue.
 */
typedef struct PriorityNode {
  void *data;
  int priority;
  struct PriorityNode *next;
} PriorityNode;

/**
 * @brief  Struct used to represent a collection of elements
 *         arranged based on their priority value
 */
typedef struct PriorityQueue {
  PriorityNode *head;
  size_t size;
  int length;
} PriorityQueue;

/**
 * @brief  Creates and returns a new priority queue
 *         with node data of size SIZE.
 *
 * @param  SIZE size of the node data.
 */
PriorityQueue* newPriorityQueue(size_t size) {
  PriorityQueue *priorityQueue = (PriorityQueue *)malloc(sizeof(PriorityQueue));
  priorityQueue->head = NULL;
  priorityQueue->size = size;
  priorityQueue->length = 0;
  return priorityQueue;
}

/**
 * @brief  Frees the priority queue and all its nodes.
 *
 * @param  PRIORITY_QUEUE the priority queue to be freed.
 */
void deletePriorityQueue(PriorityQueue *priorityQueue) {
  PriorityNode *node = priorityQueue->head;
  for (int i = 0; i < priorityQueue->length; ++i) {
    PriorityNode *prev = node;
    node = node->next;
    free(prev);
  }
  free(priorityQueue);
}

/**
 * @brief  Inserts a new node to the priority queue depending on their priority value
 *         containing the data provided inside DATA.
 *
 * @param  PRIORITY_QUEUE pointer to the priority queue.
 * @param  DATA pointer to the data to be inserted.
 * @param  PRIORITY integer value representing the priority of the inserted node.
 */
void enqueuePQ(PriorityQueue *priorityQueue, void *data, int priority) {
  PriorityNode *start = priorityQueue->head;
  PriorityNode *node = (PriorityNode *)malloc(sizeof(PriorityNode));
  node->data = malloc(priorityQueue->size);
  node->priority = priority;

  memcpy(node->data, data, priorityQueue->size);

  if (priorityQueue->head == NULL) {
    node->next = node;
    priorityQueue->head = node;
  } else {
    while (start->next != NULL && start->next->priority > priority) {
      start = start->next;
    }
    node->next = start->next;
    start->next = node;
  }

  priorityQueue->length += 1;
}

/**
 * @brief  Removes the node at the head of a priority queue and
 *         returns True if there was a node to remove.
 *         It copies the removed node data to the memory
 *         location provided in DATA.
 *         If NULL is given instead of a memory location
 *         then it replaces it with a pointer to a copy
 *         of the removed node data.
 *         If (void *)-1 is given instead of a memory
 *         location then it doesn't make any copies of
 *         the removed node data.
 *
 * @param  PRIORITY_QUEUE pointer to the priority queue.
 * @param  DATA pointer to memory location
 *         to copy the removed node data to.
 */
bool dequeuePQ(PriorityQueue *priorityQueue, void **data) {
  PriorityNode *node = priorityQueue->head;

  if (node == NULL) {
    return false;
  }

  if (data != (void **)-1) {
    if (*data == NULL) {
      *data = malloc(priorityQueue->size);
    }

    memcpy(*data, node->data, priorityQueue->size);
  }

  priorityQueue->head = (node == node->next) ? NULL : node->next;

  priorityQueue->length -= 1;

  free(node);
  return true;
}

/**
 * @brief  Returns True if there are any Nodes in the priority queue.
 *         It copies the head node data to the memory
 *         location provided in DATA. If NULL is given
 *         instead of a memory location then it replaces
 *         it with a pointer to a copy of the head node data.
 *
 * @param  PRIORITY_QUEUE to the priority queue.
 * @param  DATA pointer to memory location
 *         to copy the head node data to.
 */
bool peekPQ(PriorityQueue *priorityQueue, void **data) {
  PriorityNode *node = priorityQueue->head;

  if (node == NULL) {
    return false;
  }

  if (*data == NULL) {
    *data = malloc(priorityQueue->size);
  }

  memcpy(*data, node->data, priorityQueue->size);

  return true;
}

#endif
