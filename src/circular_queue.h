#ifndef __CIRCULAR_QUEUE_H
#define __CIRCULAR_QUEUE_H

#include "node.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief  Struct used to represent a circular collection of elements
 */
typedef struct CircularQueue {
  Node *head;
  size_t size;
  int length;
} CircularQueue;

/**
 * @brief  Creates and returns a new circular queue
 *         with node data of size SIZE.
 *
 * @param  SIZE size of the node data.
 */
CircularQueue* newCircularQueue(size_t size) {
  CircularQueue *circularQueue = (CircularQueue *)malloc(sizeof(CircularQueue));
  circularQueue->head = NULL;
  circularQueue->size = size;
  circularQueue->length = 0;
  return circularQueue;
}

/**
 * @brief  Frees the circular queue and all its nodes.
 *
 * @param  CIRCULAR_QUEUE the circular queue to be freed.
 */
void deleteCircularQueue(CircularQueue *circularQueue) {
  Node *node = circularQueue->head;
  for (int i = 0; i < circularQueue->length; ++i) {
    Node *prev = node;
    node = node->next;
    free(prev);
  }
  free(circularQueue);
}

/**
 * @brief  Inserts a new node before the head of a circular queue
 *         containing the data provided inside DATA.
 *
 * @param  CIRCULAR_QUEUE pointer to the circular queue.
 * @param  DATA pointer to the data to be inserted.
 */
void enqueueCQ(CircularQueue *circularQueue, void *data) {
  Node *node = (Node *)malloc(sizeof(Node));
  node->data = malloc(circularQueue->size);

  memcpy(node->data, data, circularQueue->size);

  if (circularQueue->head == NULL) {
    node->next = node;
    node->prev = node;
    circularQueue->head = node;
  } else {
    node->next = circularQueue->head;
    node->prev = circularQueue->head->prev;
    circularQueue->head->prev = node;
    node->prev->next = node;
  }

  circularQueue->length += 1;
}

/**
 * @brief  Removes the node at the head of a circular queue and
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
 * @param  CIRCULAR_QUEUE pointer to the circular queue.
 * @param  DATA pointer to memory location
 *         to copy the removed node data to.
 */
bool dequeueCQ(CircularQueue *circularQueue, void **data) {
  Node *node = circularQueue->head;

  if (node == NULL) {
    return false;
  }

  if (data != (void **)-1) {
    if (*data == NULL) {
      *data = malloc(circularQueue->size);
    }

    memcpy(*data, node->data, circularQueue->size);
  }

  circularQueue->head = (node == node->next) ? NULL : node->next;

  if (circularQueue->head != NULL) {
    circularQueue->head->prev = node->prev;
    node->prev->next = circularQueue->head;
  }

  circularQueue->length -= 1;

  free(node);
  return true;
}

/**
 * @brief  Returns True if there are any Nodes in the circular queue.
 *         It copies the head node data to the memory
 *         location provided in DATA. If NULL is given
 *         instead of a memory location then it replaces
 *         it with a pointer to a copy of the head node data.
 *
 * @param  CIRCULAR_QUEUE pointer to the circular queue.
 * @param  DATA pointer to memory location
 *         to copy the head node data to.
 */
bool peekCQ(CircularQueue *circularQueue, void **data) {
  Node *node = circularQueue->head;

  if (node == NULL) {
    return false;
  }

  if (*data == NULL) {
    *data = malloc(circularQueue->size);
  }

  memcpy(*data, node->data, circularQueue->size);

  return true;
}

/**
 * @brief  Returns True if there are any Nodes in the circular queue.
 *         It moves the head of the queue to the next node
 *         and copies the old head node data to the memory
 *         location provided in DATA. If NULL is given
 *         instead of a memory location then it replaces
 *         it with a pointer to a copy of the old head node data.
 *
 * @param  CIRCULAR_QUEUE pointer to the circular queue.
 * @param  DATA pointer to memory location
 *         to copy the head node data to.
 */
bool moveNext(CircularQueue *circularQueue, void **data) {
  if (peekCQ(circularQueue, data)) {
    circularQueue->head = circularQueue->head->next;
    return true;
  }

  return false;
}

/**
 * @brief  Returns True if there are any Nodes in the circular queue.
 *         It moves the head of the queue to the previous node
 *         and copies the old head node data to the memory
 *         location provided in DATA. If NULL is given
 *         instead of a memory location then it replaces
 *         it with a pointer to a copy of the old head node data.
 *
 * @param  CIRCULAR_QUEUE pointer to the circular queue.
 * @param  DATA pointer to memory location
 *         to copy the head node data to.
 */
bool movePrev(CircularQueue *circularQueue, void **data) {
  if (peekCQ(circularQueue, data)) {
    circularQueue->head = circularQueue->head->prev;
    return true;
  }

  return false;
}

#endif
