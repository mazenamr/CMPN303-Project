#include <stdbool.h>
#include <stdlib.h>

/**
 * @brief  Node used to represent one element in a deque.
 */
typedef struct Node {
  void *data;
  struct Node *next;
  struct Node *prev;
} Node;

/**
 * @brief  Deque used to represent a collection of elements
 *         that can be operated on from its top or its end.
 */
typedef struct Deque {
  struct Node *head;
  struct Node *tail;
  int size;
  int length;
} Deque;


/**
 * @brief  Inserts a new node at the top of a deque containing
 *         the data provided inside DATA.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA pointer to the data to be inserted.
 */
void push(Deque *deque, void *data) {
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
 * @brief  Removes a node from the top of a deque and
 *         returns True if there was a node to remove.
 *         It puts a pointer to a copy of the removed node data
 *         in DATA if DATA is NULL, otherwise it
 *         copies into it the removed node data.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA memory location to copy the removed node data to.
 */
bool pop(Deque *deque, void *data) {
  Node* head = deque->head;

  if (head == NULL) {
    return false;
  }

  if (data == NULL) {
    (*(void **)data) = malloc(deque->size);
  }

  for (int i = 0; i < deque->size; ++i) {
    *(char *)((char *)data + i) = *(char *)((char *)head->data + i);
  }

  deque->head = head->next;
  deque->head->prev = NULL;
  deque->length -= 1;

  free(head);
  return true;
}

/**
 * @brief  Returns True if there are any Nodes in the deque.
 *         It puts a pointer to a copy of the data in the top node
 *         in DATA if DATA is NULL, otherwise it
 *         copies into it the data in the top node.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA memory location to copy the removed node data to.
 */
bool peektop(Deque *deque, void *data) {
  Node* head = deque->head;

  if (head == NULL) {
    return false;
  }

  if (data == NULL) {
    (*(void **)data) = malloc(deque->size);
  }

  for (int i = 0; i < deque->size; ++i) {
    *(char *)((char *)data + i) = *(char *)((char *)head->data + i);
  }

  return true;
}

/**
 * @brief  Inserts a new node at the end of a deque containing
 *         the data provided inside DATA.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA pointer to the data to be inserted.
 */
void enqueue(Deque *deque, void *data) {
  Node *newNode = (Node *)malloc(sizeof(Node));
  newNode->data = malloc(deque->size);

  for (int i = 0; i < deque->size; ++i) {
    *(char *)((char *)newNode->data + i) = *(char *)((char *)data + i);
  }

  newNode->next = NULL;
  newNode->prev = deque->tail;

  if (deque->tail != NULL) {
    deque->tail->next = newNode;
  }

  deque->tail = newNode;
  deque->length += 1;

  if (deque->head == NULL) {
    deque->head = newNode;
  }
}

/**
 * @brief  Removes a node from the end of a deque and
 *         returns True if there was a node to remove.
 *         It puts a pointer to a copy of the removed node data
 *         in DATA if DATA is NULL, otherwise it
 *         copies into it the removed node data.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA memory location to copy the removed node data to.
 */
bool dequeue(Deque *deque, void *data) {
  Node* tail = deque->tail;

  if (tail == NULL) {
    return false;
  }

  if (data == NULL) {
    (*(void **)data) = malloc(deque->size);
  }

  for (int i = 0; i < deque->size; ++i) {
    *(char *)((char *)data + i) = *(char *)((char *)tail->data + i);
  }

  deque->tail = tail->prev;
  deque->tail->next = NULL;
  deque->length -= 1;

  free(tail);
  return true;
}

/**
 * @brief  Returns True if there are any Nodes in the deque.
 *         It puts a pointer to a copy of the data in the end node
 *         in DATA if DATA is NULL, otherwise it
 *         copies into it the data in the end node.
 *
 * @param  DEQUE pointer to the deque.
 * @param  DATA memory location to copy the removed node data to.
 */
bool peekend(Deque *deque, void *data) {
  Node* tail = deque->tail;

  if (tail == NULL) {
    return false;
  }

  if (data == NULL) {
    (*(void **)data) = malloc(deque->size);
  }

  for (int i = 0; i < deque->size; ++i) {
    *(char *)((char *)data + i) = *(char *)((char *)tail->data + i);
  }

  return true;
}
