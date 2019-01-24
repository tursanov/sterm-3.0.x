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
	listview_column_t* columns;
	size_t column_count;
    list_t *items;
	int top_index;
	int selected_index;
	listview_get_item_text_func_t get_item_text_func;
} listview_t;

void listview_destroy(listview_t *listview);
void listview_draw(listview_t *listview);
bool listview_focus(listview_t *listview, bool focus);
bool listview_handle(listview_t *listview, const struct kbd_event *e);
bool listview_get_data(listview_t *listview, int what, data_t *data);
bool listview_set_data(listview_t *listview, int what, const void *data, size_t data_len);

#define screen listview->control.gc

static void listview_set_selected_index(listview_t *lv, int index, bool refresh) {

}

control_t* listview_create(int id, GCPtr gc, int x, int y, int width, int height,
		listview_column_t *columns, size_t column_count,
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

    listview->columns = (listview_column_t *)malloc(sizeof(listview_column_t) * column_count);
    listview_column_t *c = listview->columns;
    listview_column_t *c1 = columns;
    for (int i = 0; i < column_count; i++, c++, c1++) {
        c->title = strdup(c1->title[i]);
        c->width = c1->width;
    }

	listview->items = items;
	listview->get_item_text_func = get_item_text_func;
    if (selected_index != -1)
	    listview_set_selected_index(listview, selected_index, false);

    return (control_t *)listview;
}

void listview_destroy(listview_t *listview) {
    listview_column_t *c = listview->columns;
    for (int i = 0; i < listview->column_count; i++, c++)
        free(c->title);
    free(listview->columns);
	free(listview);
}

static void listview_draw_column(listview_t *listview, listview_column_t *c, int x, int y, int h) {
	SetGCBounds(screen, x, y, c->width, h);
	SetTextColor(screen, clGray);
	TextOut(screen, 4, 0, c->title);
	SetBrushColor(screen, clGray);
	FillBox(screen, c->width - 2, 0, 2, h);
}

static void listview_draw_columns(listview_t *listview, int y) {
	listview_column_t *c = listview->columns;
	int x = listview->control.x;
    int y = listview->control.y;
    int w = listview->control.width;
	int h = GetTextHeight(screen) + 4;

	SetBrushColor(screen, clGray);
	FillBox(screen, x + 2, y + h, w - 4, 2);
	for (int i = 0; i < listview->column_count; i++, x += c->width, c++)
		listview_draw_column(listview, c, x, y, h);
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
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
