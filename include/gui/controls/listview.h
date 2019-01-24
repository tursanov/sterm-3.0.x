#ifndef LISTVIEW_H
#define LISTVIEW_H

#include "gui/controls/control.h"
#include "list.h"

typedef struct listview_column_t {
	char *title;
	int width;
} listview_column_t;

typedef void (*listview_get_item_text_func_t)(void *item, int index, char text[], size_t text_size);

control_t* listview_create(int id, GCPtr gc, int x, int y, int width, int height,
		list_t *items, listview_get_item_text_func_t get_item_text_func,
		int selected_index);

#endif // LISTVIEW