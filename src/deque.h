#include <stdbool.h>
#include <stdlib.h>

/**
 * @brief  Struct used to represent one element in a deque.
 */
typedef struct Node {
  void *data;
  struct Node *next;
  struct Node *prev;
} Node;

/**
 * @brief  Struct used to represent a collection of elements
 *         that can be operated on from its front or its back.
 */
typedef struct Deque {
  Node *head;
  Node *tail;
  int size;
  int length;
} Deque;

/**
 * @brief  Creates and returns a new deque
 *         with node data of size SIZE.
 *
 * @param  SIZE size of the node data.
 */
Deque* newDeque(int size) {
  Deque *deque = (Deque *)malloc(sizeof(Deque));
  deque->head = NULL;
  deque->tail = NULL;
  deque->size = size;
  deque->length = 0;
  return deque;
}

/**
 * @brief  Inserts a new node to the front of a deque containing
 *         the data provided inside DATA.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA pointer to the data to be inserted.
 */
void pushFront(Deque *deque, void *data) {
  Node *newNode = (Node *)malloc(sizeof(Node));
  newNode->data = malloc(deque->size);

  for (int i = 0; i < deque->size; ++i) {
    *(char *)((char *)newNode->data + i) = *(char *)((char *)data + i);
  }

  newNode->next = deque->head;
  newNode->prev = NULL;

  if (deque->head != NULL) {
    deque->head->prev = newNode;
  }

  deque->head = newNode;
  deque->length += 1;

  if (deque->tail == NULL) {
    deque->tail = newNode;
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
  Node *newNode = (Node *)malloc(sizeof(Node));
  newNode->data = malloc(deque->size);

  for (int i = 0; i < deque->size; ++i) {
    *(char *)((char *)newNode->data + i) = *(char *)((char *)data + i);
  }

  newNode->next = NULL;
  newNode->prev = deque->tail;

  if ((deque->tail) != NULL) {
    deque->tail->next = newNode;
  }

  deque->tail = newNode;
  deque->length += 1;

  if (deque->head == NULL) {
    deque->head = newNode;
  }
}

/**
 * @brief  Removes a node from the front of a deque and
 *         returns True if there was a node to remove.
 *         It copies the removed node data to the memory
 *         location provided in DATA. If NULL is given
 *         instead of a memory location then it replaces
 *         it with a pointer to a copy of the removed node data.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA pointer to memory location
 *         to copy the removed node data to.
 */
bool popFront(Deque *deque, void **data) {
  Node* head = deque->head;

  if (head == NULL) {
    return false;
  }

  if (*data == NULL) {
    *data = malloc(deque->size);
  }

  for (int i = 0; i < deque->size; ++i) {
    *(char *)((char *)(*data) + i) = *(char *)((char *)head->data + i);
  }

  deque->head = head->next;
  deque->head->prev = NULL;
  deque->length -= 1;

  free(head);
  return true;
}

/**
 * @brief  Removes a node from the back of a deque and
 *         returns True if there was a node to remove.
 *         It copies the removed node data to the memory
 *         location provided in DATA. If NULL is given
 *         instead of a memory location then it replaces
 *         it with a pointer to a copy of the removed node data.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA pointer to memory location
 *         to copy the removed node data to.
 */
bool popBack(Deque *deque, void **data) {
  Node* tail = deque->tail;

  if (tail == NULL) {
    return false;
  }

  if (*data == NULL) {
    *data = malloc(deque->size);
  }

  for (int i = 0; i < deque->size; ++i) {
    *(char *)((char *)(*data) + i) = *(char *)((char *)tail->data + i);
  }

  deque->tail = tail->prev;
  deque->tail->next = NULL;
  deque->length -= 1;

  free(tail);
  return true;
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
  Node* head = deque->head;

  if (head == NULL) {
    return false;
  }

  if (*data == NULL) {
    *data = malloc(deque->size);
  }

  for (int i = 0; i < deque->size; ++i) {
    *(char *)((char *)(*data) + i) = *(char *)((char *)head->data + i);
  }

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
  Node* tail = deque->tail;

  if (tail == NULL) {
    return false;
  }

  if (*data == NULL) {
    *data = malloc(deque->size);
  }

  for (int i = 0; i < deque->size; ++i) {
    *(char *)((char *)(*data) + i) = *(char *)((char *)tail->data + i);
  }

  return true;
}
