#ifndef LIST_H
#define LIST_H

#include <common.h>

typedef struct {
  void *data;
  Node *next;
  Node *prev;
} Node;

typedef struct {
  Node *head;
  Node *tail;
} List;

List *list_create();
void list_append(List *list, Node *node);
void list_add_front(List *list, Node *node);
void list_remove(List *list, Node *node);
void list_delete(List *list, Node *node);
void list_print(List *list);

#endif
