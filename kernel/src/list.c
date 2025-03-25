/* linked list helper */                                                                                                            
#include <common.h>

typedef struct list_head {                                                                                                          
  struct list_head *next, *prev;                                                                                                    
} list_head;                                                                                                                        
                                                                                                                                    
static void list_init(list_head *list) {                                                                                            
  list->next = list;                                                                                                                
  list->prev = list;                                                                                                                
}                                                                                                                                   
                                                                                                                                    
static void list_add(list_head *head, list_head *new) {                                                                             
  list_head *head_next = head->next;                                                                                                
  list_head *head_prev = head;                                                                                                      
                                                                                                                                    
  new->next = head_next;                                                                                                            
  new->prev = head_prev;                                                                                                            
  head_next->prev = new;                                                                                                            
  head_prev->next = new;                                                                                                            
}                                                                                                                                   
                                                                                                                                    
static void list_del(list_head *prev, list_head *next) {                                                                            
  next->prev = prev;                                                                                                                
  prev->next = next;                                                                                                                
} 

static void list_del_entry(list_head *entry) {
  next->prev = prev;                                                                                                                
  prev->next = next;                                                                                                               
}                                                                                                                                   
                                                                                                                                    
static void list_del_entry(list_head *entry) {                                                                                      
  list_del(entry->prev, entry->next);                                                                                               
}                                                                                                                                   
                                                                                                                                    
static bool list_empty(list_head *head) {                                                                                           
  return head->next == head && head->prev == head;                                                                                  
}    
