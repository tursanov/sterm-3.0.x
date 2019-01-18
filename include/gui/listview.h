#ifndef LISTVIEW_H
#define LISTVIEW_H

#include "list.h"

typedef struct listview_column_t {
	char *title;
	int width;
} listview_column_t;

typedef struct data_source_t {
	list_t *list;
	int (*get_text)(void *obj, int index, char *text, size_t text_size);
	void* (*new_item)(struct data_source_t *ds);
	int (*edit_item)(struct data_source_t *ds, void *obj);
	int (*remove_item)(struct data_source_t *ds, void *obj);
} data_source_t;

typedef struct listview_t {
	const char *title;
	listview_column_t* columns;
	size_t column_count;
	int top_index;
	int selected_index;
	data_source_t *data_source;
} listview_t;

listview_t *listview_create(const char *title, 
	listview_column_t* columns, 
	size_t column_count,
	data_source_t *data_source);

void listview_destroy(listview_t *listview);
int listview_execute(listview_t *listview);
void listview_draw(listview_t *listview);

#endif // LISTVIEW_H
