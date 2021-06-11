#ifndef __PRIORITY_NODE_H
#define __PRIORITY_NODE_H

/**
 * @brief  Struct used to represent one element in a collection.
 */
typedef struct PriorityNode {
  void *data;
  int priority;
  struct PriorityNode *next;
} PriorityNode;

#endif