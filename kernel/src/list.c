/* linked list helper */                                                                                                            
#include <common.h>

typedef struct list_head {
  struct list_head *next, *prev; 
} list_head;                                                                                                          
void list_init(list_head *list) {
  list->next = list;
  list->prev = list;
}
 
void list_add(list_head *head, list_head *list) {
  list_head *head_next = head->next;
  list_head *head_prev = head;
  list->next = head_next;
  list->prev = head_prev;
  head_next->prev = list;
  head_prev->next = list;
}

void list_del(list_head *prev, list_head *next) {
  next->prev = prev;
  prev->next = next;
} 

void list_del_entry(list_head *entry) {
  list_del(entry->prev, entry->next);
}

int list_empty(list_head *list) {
  return list->next == list && list->prev == list;
} 
