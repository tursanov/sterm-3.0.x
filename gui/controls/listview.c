#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "sysdefs.h"
#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/controls/listview.h"

typedef struct listview_t {
	control_t control;
	list_t *items;
	listview_get_item_text_func_t get_item_text_func;
	int selected_index;
} listview_t;

void listview_destroy(listview_t *listview);
void listview_draw(listview_t *listview);
bool listview_focus(listview_t *listview, bool focus);
bool listview_handle(listview_t *listview, const struct kbd_event *e);
bool listview_get_data(listview_t *listview, int what, data_t *data);
bool listview_set_data(listview_t *listview, int what, const void *data, size_t data_len);

#define screen listview->control.gc

control_t* listview_create(int id, GCPtr gc, int x, int y, int width, int height,
		list_t *items, listview_get_item_text_func_t get_item_text_func, 
		int selected_index) {
    listview_t *listview = (listview_t *)malloc(sizeof(listview_t));
    control_api_t api = {
		(void (*)(struct control_t *))listview_destroy,
		(void (*)(struct control_t *))listview_draw,
		(bool (*)(struct control_t *, bool))listview_focus,
		(bool (*)(struct control_t *, const struct kbd_event *))listview_handle,
		(bool (*)(struct control_t *, int, data_t *))listview_get_data,
		(bool (*)(struct control_t *control, int, const void *, size_t))listview_set_data
    };

    control_init(&listview->control, id, gc, x, y, width, height, &api);

	listview->items = items;
	listview->get_item_text_func = get_item_text_func;
	listview->selected_index = selected_index;

    return (control_t *)listview;
}

void listview_destroy(listview_t *listview) {
	free(listview);
}

void listview_draw(listview_t *listview) {
	Color borderColor;
	Color bgColor;
	Color fgColor;

	if (listview->control.focused) {
		borderColor = clRopnetDarkBrown;
		bgColor = clRopnetBrown;
		fgColor = clBlack;
	} else {
		borderColor = RGB(184, 184, 184);
		bgColor = RGB(200, 200, 200);
		fgColor = RGB(32, 32, 32);
	}
	
	fill_rect(screen, listview->control.x, listview->control.y,
		listview->control.width, listview->control.height, BORDER_WIDTH,
		borderColor, bgColor);

	SetGCBounds(screen, listview->control.x + BORDER_WIDTH, listview->control.y + BORDER_WIDTH,
		listview->control.width - BORDER_WIDTH * 2, listview->control.height - BORDER_WIDTH * 2);

	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

bool listview_focus(listview_t *listview, bool focus) {
	listview->control.focused = focus;
	listview_draw(listview);
	return true;
}

bool listview_handle(listview_t *listview, const struct kbd_event *e) {
	switch (e->key) {
	case KEY_DOWN:
	case KEY_UP:
	case KEY_HOME:
	case KEY_END:
	case KEY_PGUP:
	case KEY_PGDN:
		return true;
	default:
		break;
	}
	return false;
}

bool listview_get_data(listview_t *listview, int what, data_t *data) {
	switch (what) {
	case 0:
		data->data = (void *)listview->selected_index;
		data->size = 4;
		return true;
	}
	return false;
}

bool listview_set_data(listview_t *listview, int what, const void *data, size_t data_len) {
	switch (what) {
	case 0:
		listview->selected_index = (int32_t)data;
		listview_draw(listview);
		return true;
	}
	return false;
}
