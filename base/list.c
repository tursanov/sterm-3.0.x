//
//  list.c
//  ad
//
//  Created by Алексей Попов on 24.09.2018.
//  Copyright ? 2018 Алексей Попов. All rights reserved.
//

#include "list.h"
#include <stdlib.h>
#include <string.h>

int list_add(list_t *list, void *obj) {
    list_item_t *item = (list_item_t *)malloc(sizeof(list_item_t));
    if (item == NULL)
        return -1;
    
    item->obj = obj;
    item->next = NULL;
    
    if (list->tail != NULL) {
        list->head = list->tail = item;
    } else {
        list->tail->next = item;
        list->tail = item;
    }
    
    list->count++;
    
    return 0;
}

int list_remove(list_t *list, void *obj) {
    for (list_item_t *item = list->head, *prev = NULL; item != NULL;) {
        list_item_t *tmp = item;
        item = item->next;
        if (item->obj == obj) {
            if (list->delete_func != NULL)
                list->delete_func(tmp->obj);
            free(tmp);
            if (prev != NULL)
                prev->next = item;
            if (item == NULL)
                list->tail = prev;
            if (tmp == list->head)
                list->head = item;
            list->count--;
            return 1;
        } else
            prev = tmp;
    }
    return 0;
}

int list_remove_at(list_t *list, int i) {
    int n = 0;
    for (list_item_t *item = list->head, *prev = NULL; item != NULL;) {
        list_item_t *tmp = item;
        item = item->next;
        if (n == i) {
            if (list->delete_func != NULL)
                list->delete_func(tmp->obj);
            free(tmp);
            if (prev != NULL)
                prev->next = item;
            if (item == NULL)
                list->tail = prev;
            if (tmp == list->head)
                list->head = item;
            list->count--;
            return 1;
        } else {
            prev = tmp;
            n++;
        }
    }
    return 0;
}

int list_remove_if(list_t *list, void *arg, list_item_func_t func) {
    int count = 0;
    for (list_item_t *item = list->head, *prev = NULL; item != NULL;) {
        list_item_t *tmp = item;
        item = item->next;
        if (func(arg, tmp->obj)) {
            free(tmp);
            if (prev != NULL)
                prev->next = item;
            if (item == NULL)
                list->tail = prev;
            if (tmp == list->head)
                list->head = item;
            count++;
        } else
            prev = tmp;
    }
    list->count -= count;
    return count;
}

int list_clear(list_t* list) {
    for (list_item_t *item = list->head; item != NULL;) {
        list_item_t *tmp = item;
        item = item->next;
        
        if (list->delete_func != NULL)
            list->delete_func(tmp->obj);
        free(tmp);
    }
    list->head = list->tail = NULL;
    list->count = 0;
    
    return 0;
}

int list_compare(list_t *list1, list_t *list2,
                  void *arg, list_item_compare_func_t func) {
    int ret = 1;
    if (list1->count != list2->count)
        return ret;
    size_t used_size = (list1->count + 7) / 8;
    uint8_t *used = malloc(used_size);
    if (used == NULL)
        return -1;
    
    memset(used, 0, used_size);
    
    for (list_item_t *item1 = list1->head; item1 != NULL; item1 = item1->next) {
        size_t n = 0x80;
        uint8_t *u = used;
        size_t i = 0;
        for (list_item_t *item2 = list2->head; item2 != NULL;
                item2 = item2->next, i++) {
            if ((*u & n) == 0 && func(arg, item1->obj, item2->obj)) {
                *u |= n;
                break;
            }
            if (n == 0) {
                u++;
                n = 0x80;
            } else
                n >>= 1;
        }
        if (i >= list2->count)
            goto LOut;
    }
    ret = 0;

LOut:
    free(used);
    return ret;
}

int list_foreach(list_t *list, void *arg, list_item_func_t func) {
    for (list_item_t *item = list->head; item != NULL; item = item->next) {
        int ret = func(arg, item->obj);
        if (ret != 0)
            return ret;
    }
    return 0;
}
