typedef struct list_head {
  struct list_head *next, *prev;
} list_head;

void list_init(list_head *list);
void list_add(list_head *head, list_head *list);
void list_del_entry(list_head *entry);
void list_empty(list_head *list);
