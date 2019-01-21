#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "sysdefs.h"
#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/forms.h"
#include "gui/dialog.h"
#include "gui/listview.h"

static FontPtr fnt = NULL;
static GCPtr screen = NULL;
static int result = 0;
static int item_h;
static int items_y;
static int items_bottom;
static int max_items;
static int active_element = 0;
#define GAP	10
#define BUTTON_WIDTH	100
#define BUTTON_HEIGHT	30

listview_t *listview_create(const char *title, 
		listview_column_t* columns, size_t column_count,
		data_source_t *data_source) 
{
	listview_t *listview = (listview_t *)malloc(sizeof(listview_t));

	listview->top_index = listview->selected_index = data_source->list->head ? 0 : -1;

	listview->data_source = data_source;
	listview->title = title;
	listview->columns = columns;
	listview->column_count = column_count;

	return listview;
}

void listview_destroy(listview_t *listview) {
	free(listview);
}

extern void draw_title(GCPtr screen, FontPtr fnt, const char *title);
extern void fill_rect(GCPtr screen, int x, int y, int width, int height, int border_width,
		Color border_color, int bg_color);

static void listview_draw_column(listview_column_t *c, int x, int y, int h) {
	SetGCBounds(screen, x, y, c->width, h);
	SetTextColor(screen, clGray);
	TextOut(screen, 4, 0, c->title);
	SetBrushColor(screen, clGray);
	FillBox(screen, c->width - 2, 0, 2, h);
}

static void listview_draw_columns(listview_t *listview, int y) {
	listview_column_t *c = listview->columns;
	int x = GAP + 2;
	int h = fnt->max_height + 1;

	for (int i = 0; i < listview->column_count; i++, x += c->width, c++)
		listview_draw_column(c, x, y, h);
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
	SetBrushColor(screen, clGray);
	FillBox(screen, GAP + 2, y + h, DISCX - GAP*2 - 4, 2);
}

static list_item_t *listview_get_top_item(listview_t *listview, int *idx) {
	list_item_t *item = listview->data_source->list->head;
	int n = 0;
	for (n = 0; n < listview->top_index && item; n++, item = item->next);

	if (!item) {
		item = listview->data_source->list->head;
		if (item)
			listview->top_index = listview->selected_index = 0;
		else
			listview->top_index = listview->selected_index = -1;
	}

	*idx = n;

	return item;
}

static list_item_t *listview_get_selected_item(listview_t *listview) {
	list_item_t *item = listview->data_source->list->head;
	for (int n = 0; n < listview->selected_index && item; n++, item = item->next);
	return item;
}

static void listview_draw_row(listview_t *listview, int index, list_item_t *item, 
		int x, int y, int h) {
	listview_column_t *c = listview->columns;

	SetGCBounds(screen, x + 2, y, DISCX - x*2  - 4, h);
	SetBrushColor(screen, listview->selected_index == index ? 
			(active_element == 0 ? clNavy : clGray) : clSilver);
	SetTextColor(screen, listview->selected_index == index ? clWhite : clBlack);
	FillBox(screen, 0, 0, DISCX - x*2 - 4, h);

	for (int j = 0; j < listview->column_count; j++, x += c->width, c++) {
		char text[256];
		if (listview->data_source->get_text(item->obj, j, text, sizeof(text) - 1) == 0) {
			SetGCBounds(screen, x, y, c->width, h);
			TextOut(screen, 4, 0, text);
		}
	}
}

static void listview_redraw_rows(listview_t *listview, int index1, int index2) {
	int x = GAP;
	int y = items_y;
	int n;
	int i1, i2;
	list_item_t *item = listview_get_top_item(listview, &n);

	if (index1 < index2) {
		i1 = index1;
		i2 = index2;
	} else {
		i1 = index2;
		i2 = index1;
	}

	for (; item && n <= i2; item = item->next, n++) {
		if (n == i1 || n == i2) {
			listview_draw_row(listview, n, item, x, y, item_h);
		}
		y += item_h;
	}
}

void listview_draw_items(listview_t *listview) {
	int y = items_y;

	if (listview->top_index >= 0) {
		int n;
		list_item_t *item = listview_get_top_item(listview, &n);

		for (; item && y + item_h < items_bottom; item = item->next, n++) {
			listview_draw_row(listview, n, item, GAP, y, item_h);
			y += item_h;
		}
	}
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

static void listview_draw_buttons() {
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
	draw_button(screen, fnt, GAP + (BUTTON_WIDTH + GAP)*0, items_bottom + GAP, 
			BUTTON_WIDTH, BUTTON_HEIGHT, "Добавить", active_element == 1);
	draw_button(screen, fnt, GAP + (BUTTON_WIDTH + GAP)*1, items_bottom + GAP,
			BUTTON_WIDTH, BUTTON_HEIGHT, "Изменить", active_element == 2);
	draw_button(screen, fnt, GAP + (BUTTON_WIDTH + GAP)*2, items_bottom + GAP,
			BUTTON_WIDTH, BUTTON_HEIGHT, "Удалить", active_element == 3);
	draw_button(screen, fnt, DISCX - BUTTON_WIDTH - GAP, items_bottom + GAP,
			BUTTON_WIDTH, BUTTON_HEIGHT, "Закрыть", active_element == 4);
}

void listview_draw(listview_t *listview) {
	int x = GAP;
	int y = 0;

	SetGCBounds(screen, 0, 0, DISCX, DISCY);
	ClearGC(screen, clSilver);
	draw_title(screen, fnt, listview->title);
	y += 30;

	fill_rect(screen, x, y, DISCX - GAP*2, items_bottom - y, 2, clGray, -1);
	listview_draw_columns(listview, y + 2);
	listview_draw_items(listview);
	listview_draw_buttons();
}

static void listview_set_selected_index(listview_t *listview, int index) {
	if (listview->data_source->list->head) {
		if (index < 0)
			index = 0;
		else if (index >= listview->data_source->list->count)
			index = listview->data_source->list->count - 1;

		if (index != listview->selected_index) {
			int old = listview->selected_index;
			listview->selected_index = index;

			int y = items_y + (index - listview->top_index) * item_h;

			if (y < items_y) {
				listview->top_index = index;
				listview_draw_items(listview);
			} else if (y + item_h > items_bottom) {
				listview->top_index = index - max_items;
				listview_draw_items(listview);
			} else {
				listview_redraw_rows(listview, old, index);
			}
		}
	}
}

static void listview_next_element(listview_t *listview, bool prev) {
	int old = active_element;
	if (!prev) {
		if (active_element == 4)
			active_element = 0;
		else
			active_element++;
	} else {
		if (active_element == 0)
			active_element = 4;
		else
			active_element--;
	}

	if (old == 0 || active_element == 0)
		listview_redraw_rows(listview, listview->selected_index, -1);
	listview_draw_buttons();
}

static void listview_new_action(listview_t *listview) {
	void *obj;
	if ((obj = listview->data_source->new_item(listview->data_source))) {
		list_add(listview->data_source->list, obj);

		listview->selected_index = listview->data_source->list->count - 1;
		listview->top_index = listview->selected_index - max_items;
		if (listview->top_index < 0)
			listview->top_index = 0;
	}
	listview_draw(listview);
}

static void listview_edit_action(listview_t *listview) {
	if (listview->data_source->list->head) {
		list_item_t *item = listview_get_selected_item(listview);
		if (item) {
			if ((listview->data_source->edit_item(listview->data_source, item->obj))) {
			}
			listview_draw(listview);
		}
	}
}

static void listview_delete_action(listview_t *listview) {
	if (listview->data_source->list->head) {
		list_item_t *item = listview_get_selected_item(listview);

		if (item) {
			if (message_box("Уведомление", "Выбранный элемент будет удален\nПродолжить?",
						dlg_yes_no, 0, al_center) == DLG_BTN_YES) {
				if (listview->data_source->remove_item(listview->data_source, item->obj) == 0) {
					list_remove(listview->data_source->list, item->obj);

					if (listview->selected_index == listview->data_source->list->count) {
						listview->selected_index--;
						if (listview->top_index > listview->selected_index)
							listview->top_index = listview->selected_index;
					}

					if (listview->top_index > 0)
						listview->top_index--;

					listview_draw(listview);
				}
			}
		}
	}
}

static bool listview_action(listview_t *listview) {
	switch (active_element) {
		case 0:
			if (listview->selected_index >= 0)
				listview_edit_action(listview);
			else
				listview_new_action(listview);
			break;
		case 1:
			listview_new_action(listview);
			break;
		case 2:
			listview_edit_action(listview);
			break;
		case 3:
			listview_delete_action(listview);
			break;
		default:
			result = 0;
			return false;
	}
	return true;
}

extern int kbd_lang_ex;
extern int kbd_get_char_ex(int key);

static bool listview_process(listview_t *listview, const struct kbd_event *_e) {
	struct kbd_event e = *_e;
	
	if (e.key == KEY_CAPS && e.pressed && !e.repeated) {
		if (kbd_lang_ex == lng_rus)
			kbd_lang_ex = lng_lat;
		else
			kbd_lang_ex = lng_rus;
	}

	e.ch = kbd_get_char_ex(e.key);

	if (e.pressed) {
		switch (e.key) {
			case KEY_ESCAPE:
				result = 0;
				return false;
			case KEY_DOWN:
				listview_set_selected_index(listview, listview->selected_index + 1);
				break;
			case KEY_UP:
				listview_set_selected_index(listview, listview->selected_index - 1);
				break;
			case KEY_PGUP:
				listview_set_selected_index(listview, listview->selected_index - max_items);
				break;
			case KEY_PGDN:
				listview_set_selected_index(listview, listview->selected_index + max_items);
				break;
			case KEY_HOME:
				listview_set_selected_index(listview, 0);
				break;
			case KEY_END:
				listview_set_selected_index(listview, listview->data_source->list->count - 1);
				break;
			case KEY_TAB:
				listview_next_element(listview, (e.shift_state & SHIFT_SHIFT) != 0);
				break;
			case KEY_ENTER:
				return listview_action(listview);
			case KEY_DEL:
				listview_delete_action(listview);
		}
	}

	return true;
}

int listview_execute(listview_t *listview) {
	struct kbd_event e;

	fnt = CreateFont(_("fonts/terminal10x18.fnt"), false);
   	screen = CreateGC(0, 0, DISCX, DISCY);
    SetFont(screen, fnt);

	item_h = fnt->max_height + 2;
	items_y = 30 + item_h + 4;
	items_bottom = DISCY - 100;
	max_items = ((items_bottom - items_y) / item_h) - 1;

	listview_draw(listview);

	kbd_flush_queue();
	do {
		kbd_get_event(&e);
	} while (listview_process(listview, &e));

	DeleteFont(fnt);
	DeleteGC(screen);

	return result;
}

