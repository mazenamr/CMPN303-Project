/**
 * @brief  Struct used to represent one element in a collection.
 */
typedef struct Node {
  void *data;
  struct Node *next;
  struct Node *prev;
} Node;