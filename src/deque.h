#ifndef __DEQUE_H
#define __DEQUE_H

#include "node.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief  Struct used to represent a collection of elements
 *         that can be operated on from its front or its back.
 */
typedef struct Deque {
  Node *head;
  Node *tail;
  size_t size;
  int length;
} Deque;

/**
 * @brief  Creates and returns a new deque
 *         with node data of size SIZE.
 *
 * @param  SIZE size of the node data.
 */
Deque* newDeque(size_t size) {
  Deque *deque = (Deque *)malloc(sizeof(Deque));
  deque->head = NULL;
  deque->tail = NULL;
  deque->size = size;
  deque->length = 0;
  return deque;
}

/**
 * @brief  Frees the deque and all its nodes.
 *
 * @param  DEQUE the deque to be freed.
 */
void deleteDeque(Deque *deque) {
  Node *node = deque->head;
  for (int i = 0; i < deque->length; ++i) {
    Node *prev = node;
    node = node->next;
    free(prev->data);
    free(prev);
  }
  free(deque);
}

/**
 * @brief  Inserts a new node to the front of a deque containing
 *         the data provided inside DATA.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA pointer to the data to be inserted.
 */
void pushFront(Deque *deque, void *data) {
  Node *node = (Node *)malloc(sizeof(Node));
  node->data = malloc(deque->size);

  memcpy(node->data, data, deque->size);

  node->next = deque->head;
  node->prev = NULL;

  if (deque->head != NULL) {
    deque->head->prev = node;
  }

  deque->head = node;
  deque->length += 1;

  if (deque->tail == NULL) {
    deque->tail = node;
  }
}

/**
 * @brief  Inserts a new node to the back of a deque containing
 *         the data provided inside DATA.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA pointer to the data to be inserted.
 */
void pushBack(Deque *deque, void *data) {
  Node *node = (Node *)malloc(sizeof(Node));
  node->data = malloc(deque->size);

  memcpy(node->data, data, deque->size);

  node->next = NULL;
  node->prev = deque->tail;

  if ((deque->tail) != NULL) {
    deque->tail->next = node;
  }

  deque->tail = node;
  deque->length += 1;

  if (deque->head == NULL) {
    deque->head = node;
  }
}

/**
 * @brief  Removes the node at the front of a deque and
 *         returns True if there was a node to remove.
 *
 * @param  DEQUE pointer to the deque.
 */
bool removeFront(Deque *deque) {
  Node* node = deque->head;

  if (node == NULL) {
    return false;
  }

  deque->head = node->next;
  deque->length -= 1;

  if (deque->length) {
    deque->head->prev = NULL;
    if (deque->length == 1) {
      deque->tail->prev = NULL;
    }
  } else {
    deque->tail = NULL;
  }

  free(node);
  return true;
}

/**
 * @brief  Removes the node at the back of a deque and
 *         returns True if there was a node to remove.
 *
 * @param  DEQUE pointer to the deque.
 */
bool removeBack(Deque *deque) {
  Node* node = deque->tail;

  if (node == NULL) {
    return false;
  }

  deque->tail = node->prev;
  deque->length -= 1;

  if (deque->length) {
    deque->tail->next = NULL;
    if (deque->length == 1) {
      deque->head->next = NULL;
    }
  } else {
    deque->head = NULL;
  }

  free(node);
  return true;
}

/**
 * @brief  Removes the node at the front of a deque and
 *         returns True if there was a node to remove.
 *         It copies the removed node data to the memory
 *         location provided in DATA.
 *         If NULL is given instead of a memory location
 *         then it replaces it with a pointer to a copy
 *         of the removed node data.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA pointer to memory location
 *         to copy the removed node data to.
 */
bool popFront(Deque *deque, void **data) {
  Node* node = deque->head;

  if (node == NULL) {
    return false;
  }

  if (*data == NULL) {
    *data = malloc(deque->size);
  }

  memcpy(*data, node->data, deque->size);

  return removeFront(deque);
}

/**
 * @brief  Removes the node at the back of a deque and
 *         returns True if there was a node to remove.
 *         It copies the removed node data to the memory
 *         location provided in DATA.
 *         If NULL is given instead of a memory location
 *         then it replaces it with a pointer to a copy
 *         of the removed node data.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA pointer to memory location
 *         to copy the removed node data to.
 */
bool popBack(Deque *deque, void **data) {
  Node* node = deque->tail;

  if (node == NULL) {
    return false;
  }

  if (*data == NULL) {
    *data = malloc(deque->size);
  }

  memcpy(*data, node->data, deque->size);

  return removeBack(deque);
}

/**
 * @brief  Returns True if there are any Nodes in the deque.
 *         It copies the front node data to the memory
 *         location provided in DATA. If NULL is given
 *         instead of a memory location then it replaces
 *         it with a pointer to a copy of the front node data.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA pointer to memory location
 *         to copy the front node data to.
 */
bool peekFront(Deque *deque, void **data) {
  Node* node = deque->head;

  if (node == NULL) {
    return false;
  }

  if (*data == NULL) {
    *data = malloc(deque->size);
  }

  memcpy(*data, node->data, deque->size);

  return true;
}

/**
 * @brief  Returns True if there are any Nodes in the deque.
 *         It copies the back node data to the memory
 *         location provided in DATA. If NULL is given
 *         instead of a memory location then it replaces
 *         it with a pointer to a copy of the back node data.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA pointer to memory location
 *         to copy the back node data to.
 */
bool peekBack(Deque *deque, void **data) {
  Node* node = deque->tail;

  if (node == NULL) {
    return false;
  }

  if (*data == NULL) {
    *data = malloc(deque->size);
  }

  memcpy(*data, node->data, deque->size);

  return true;
}

#endif
