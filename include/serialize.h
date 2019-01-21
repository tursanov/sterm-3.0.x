#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "list.h"

int save_string(FILE *f, char *s);
int load_string(FILE *f, char **ret);
int save_int(FILE *f, uint64_t v, size_t size);
int load_int(FILE *f, uint64_t *v, size_t size);

typedef void * (*load_item_func_t)(FILE *f);
int save_list(FILE *f, list_t *list, list_item_func_t save_item_func);
int load_list(FILE *f, list_t *list, load_item_func_t load_item_func);

#define SAVE_INT(f, v) save_int((f), (v), sizeof(v))
#define LOAD_INT(f, v) load_int((f), (uint64_t *)&(v), sizeof(v))

#endif // SERIALIZE_H
