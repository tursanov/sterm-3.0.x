#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "sysdefs.h"
#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/controls/listbox.h"

typedef struct listbox_t {
	control_t control;
	list_t *items;
	listbox_get_item_text_func_t get_item_text_func;
	int selected_index;
} listbox_t;

void listbox_destroy(listbox_t *listbox);
void listbox_draw(listbox_t *listbox);
bool listbox_focus(listbox_t *listbox, bool focus);
bool listbox_handle(listbox_t *listbox, const struct kbd_event *e);
bool listbox_get_data(listbox_t *listbox, int what, data_t *data);
bool listbox_set_data(listbox_t *listbox, int what, const void *data, size_t data_len);

#define screen listbox->control.gc

control_t* listbox_create(int id, GCPtr gc, int x, int y, int width, int height,
		list_t *items, listbox_get_item_text_func_t get_item_text_func, 
		int selected_index) {
    listbox_t *listbox = (listbox_t *)malloc(sizeof(listbox_t));
    control_api_t api = {
		(void (*)(struct control_t *))listbox_destroy,
		(void (*)(struct control_t *))listbox_draw,
		(bool (*)(struct control_t *, bool))listbox_focus,
		(bool (*)(struct control_t *, const struct kbd_event *))listbox_handle,
		(bool (*)(struct control_t *, int, data_t *))listbox_get_data,
		(bool (*)(struct control_t *control, int, const void *, size_t))listbox_set_data
    };

    control_init(&listbox->control, id, gc, x, y, width, height, &api);

	listbox->items = items;
	listbox->get_item_text_func = get_item_text_func;
	listbox->selected_index = selected_index;

    return (control_t *)listbox;
}

void listbox_destroy(listbox_t *listbox) {
	free(listbox);
}

void listbox_draw(listbox_t *listbox) {
	Color borderColor;
	Color bgColor;
	Color fgColor;

	if (listbox->control.focused) {
		borderColor = clRopnetDarkBrown;
		bgColor = clRopnetBrown;
		fgColor = clBlack;
	} else {
		borderColor = RGB(184, 184, 184);
		bgColor = RGB(200, 200, 200);
		fgColor = RGB(32, 32, 32);
	}
	
	fill_rect(screen, listbox->control.x, listbox->control.y,
		listbox->control.width, listbox->control.height, BORDER_WIDTH,
		borderColor, bgColor);

	SetGCBounds(screen, listbox->control.x + BORDER_WIDTH, listbox->control.y + BORDER_WIDTH,
		listbox->control.width - BORDER_WIDTH * 2, listbox->control.height - BORDER_WIDTH * 2);

	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

bool listbox_focus(listbox_t *listbox, bool focus) {
	listbox->control.focused = focus;
	listbox_draw(listbox);
	return true;
}

bool listbox_handle(listbox_t *listbox, const struct kbd_event *e) {
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

bool listbox_get_data(listbox_t *listbox, int what, data_t *data) {
	switch (what) {
	case 0:
		data->data = (void *)listbox->selected_index;
		data->size = 4;
		return true;
	}
	return false;
}

bool listbox_set_data(listbox_t *listbox, int what, const void *data, size_t data_len) {
	switch (what) {
	case 0:
		listbox->selected_index = (int32_t)data;
		listbox_draw(listbox);
		return true;
	}
	return false;
}
