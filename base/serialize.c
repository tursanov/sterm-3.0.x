#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "serialize.h"

int save_string(FILE *f, char *s) {
    size_t len = s != NULL ? strlen(s) : 0;
    if (fwrite(&len, sizeof(len), 1, f) != 1)
        return -1;
    if (len > 0 && fwrite(s, len, 1, f) != 1)
        return -1;
    return 0;
}

int load_string(FILE *f, char **ret) {
    size_t len;
    if (fread(&len, sizeof(len), 1, f) != 1)
        return -1;
	if (len > 0) {
		char *s = (char *)malloc(len + 1);
		if (s == NULL)
			return -1;

		if (fread(s, len, 1, f) != 1) {
			free(s);
			return -1;
		}
		s[len] = 0;

		*ret = s;
	} else
		*ret = NULL;
    
    return 0;
}

int save_int(FILE *f, uint64_t v, size_t size) {
    if (fwrite(&v, size, 1, f) != 1)
        return -1;
    return 0;
}

int load_int(FILE *f, uint64_t *v, size_t size) {
    if (fread(v, size, 1, f) != 1)
        return -1;
    return 0;
}

int save_list(FILE *f, list_t *list, list_item_func_t save_item_func) {
    if (SAVE_INT(f, list->count) < 0 ||
        list_foreach(list, f, save_item_func) < 0)
        return -1;
    return 0;
}

int load_list(FILE *f, list_t *list, load_item_func_t load_item_func) {
    size_t count = 0;
    if (LOAD_INT(f, count) < 0)
        return -1;
    
    for (size_t i = 0; i < count; i++) {
        void *obj = load_item_func(f);
        if (obj == NULL)
            return -1;
        if (list_add(list, obj) != 0)
            return -1;
    }
    return 0;
}

