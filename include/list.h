//
//  list.h
//  ad
//
//  Created by Алексей Попов on 24.09.2018.
//  Copyright © 2018 Алексей Попов. All rights reserved.
//

#ifndef list_h
#define list_h

#include <stddef.h>
#include <stdint.h>

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
int list_remove(list_t *list, void *obj);
int list_remove_at(list_t *list, int i);
int list_remove_if(list_t *list, void *arg, list_item_func_t func);
int list_clear(list_t* list);
int list_compare(list_t *list1, list_t *list2,
                  void *arg, list_item_compare_func_t func);
int list_foreach(list_t* list, void *arg, list_item_func_t func);

#define LIST_ITEM(i, type) ((type)((i)->obj))

#endif /* list_h */
