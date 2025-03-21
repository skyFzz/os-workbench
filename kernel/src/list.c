/* Generic doubly-linked list implementation */

#include <common.h>
#include <list.h>

List *list_create() {
  List *res = (List *)malloc(sizeof(List));
  assert(res != NULL);
  res->head = res->tail = NULL;
  return res;
}

void list_append(List *list, Node *node) {
  if (!list) {
    list.head = list.tail = node;
  } else {
    node->prev = list->tail;
    list->tail->next = node;
    list->tail = node;
  }
}

void list_add_front(List *list, Node *node) {
  if (!list) {
    list.head = list.tail = node;
  } else {
    node->next = list->head;
    list->head->prev = node;
    list->head = node;
  }
}

Node *list_remove(List *list) {
  if (!list) {
    printf("Error: list_remove at list.c");
    halt(1);
  }
  
  Node *res = list->tail;
  list->tail = res->prev;
  res->prev->next = NULL;
  return res;
}

void list_delete(List *list, Node *node) {
  if (!list) {
    printf("Error: list_delete at list.c");
    halt(1);
  }
  
  if (node->prev) node->prev->next = node->next;
  else {
    list->head = node->next;
    node->next->prev = NULL;
  }
  
  if (node->next) node->next->prev = node->prev;
  else {
    list->tail = node->prev;
    node->prev->next = NULL;
  }
}

void list_print(List *list) {
  if (!list) {
    printf("Empty list :(\n");
  } else {
    for (Node *tmp = list->head; tmp; tmp = tmp->next) {
      printf("not implemented\n");
    }
  }
}
