#ifndef list_h
#define list_h

#include <stddef.h>

typedef struct list_item_t {
    struct list_item_t *next;
    void *obj;
} list_item_t;

typedef void (*list_item_delete_func_t)(void *obj);
typedef int (*list_item_func_t)(void *arg, void *obj);
typedef int (*list_item_compare_func_t)(void *arg, void *x, void *y);

typedef struct list_t {
    struct list_item_t *head;
    struct list_item_t *tail;
    size_t count;
    list_item_delete_func_t delete_func;
} list_t;

int list_add(list_t *list, void *obj);
void list_remove_item(list_t *list, list_item_t *p, list_item_t *i);
int list_remove(list_t *list, void *obj);
int list_remove_at(list_t *list, int i);
int list_remove_if(list_t *list, void *arg, list_item_func_t func);
int list_clear(list_t* list);
int list_compare(list_t *list1, list_t *list2,
                  void *arg, list_item_compare_func_t func);
int list_foreach(list_t* list, void *arg, list_item_func_t func);

#define LIST_ITEM(i, type) ((type *)((i)->obj))

typedef struct list_it_t {
    list_t *list;
    list_item_t *i;
    list_item_t *p;
} list_it_t;

#define LIST_IT(list) { (list), (list)->head, NULL }
#define LIST_IT_END(it) ((it).i != NULL)
#define LIST_IT_OBJ(it, type) ((type *)((it).i->obj))
static inline void list_it_next(list_it_t *it) {
    it->p = it->i;
    it->i = it->i->next;
}
void list_it_remove(list_it_t *it);

#endif /* list_h */
