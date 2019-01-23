#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "sysdefs.h"
#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/controls/lang.h"
#include "gui/controls/window.h"

typedef struct {
	int id;
	int x;
   	int y;
	char *text;
} label_t;

struct window_t {
	bool own_gc;
	GCPtr gc;
	char *title;
	list_t labels;
	list_t idlabels;
	list_t controls;
	list_item_t *focused;
	list_item_t *idfind;
	int dialog_result;
};

static void label_destroy(label_t *l) {
	free(l->text);
	free(l);
}

window_t *window_create(GCPtr gc, const char *title) {
	window_t *w = (window_t *)malloc(sizeof(window_t));
	memset(w, 0, sizeof(window_t));

	w->dialog_result = -1;

	if (gc == NULL) {
		FontPtr fnt = CreateFont(_("fonts/terminal10x18.fnt"), false);
	   	w->gc = CreateGC(0, 0, DISCX, DISCY);
    	SetFont(w->gc, fnt);
		w->own_gc = true;
	} else
		w->own_gc = false;

	w->controls.delete_func = (list_item_delete_func_t)control_destroy;
	w->labels.delete_func = (list_item_delete_func_t)label_destroy;
	w->idlabels.delete_func = (list_item_delete_func_t)label_destroy;
	w->title = strdup(title);
	return w;
}

void window_destroy(window_t *w) {
	list_clear(&w->controls);
	list_clear(&w->labels);
	free(w->title);
	if (w->own_gc) {
		FontPtr fnt = SetFont(w->gc, NULL);
		DeleteFont(fnt);
		DeleteGC(w->gc);
	}
	free(w);
}


GCPtr window_get_gc(window_t *w) {
	return w->gc;
}

void window_set_dialog_result(window_t *w, int result) {
	w->dialog_result = result;
}

void window_add_control(window_t *w, control_t *c) {
	list_add(&w->controls, c);
	control_parent_t p = { w, (draw_func_t)window_draw };
	control_set_parent(c, &p);
	if (w->controls.count == 1) {
		w->focused = w->controls.head;
		c->focused = true;	
	}
}

void window_add_label_with_id(window_t *w, int id, int x, int y, const char *text) {
	label_t *label = (label_t *)malloc(sizeof(label_t));
	label->id = id;
	label->x = x;
	label->y = y;
	label->text = strdup(text);
	if (id != -1)
		list_add(&w->idlabels, label);
	else
		list_add(&w->labels, label);
}

void window_add_label(window_t *w, int x, int y, const char *text) {
	window_add_label_with_id(w, -1, x, y, text);
}


void window_set_label_text(window_t *w, int id, const char *text) {
	for (list_item_t *li = w->idlabels.head; li; li = li->next) {
		label_t *l = LIST_ITEM(li, label_t);
		if (l->id == id) {
			free(l->text);
			l->text = strdup(text);
			window_draw(w);
			break;
		}
	}
}

static void draw_title(GCPtr screen, const char *title) {
#define GAP	10
	if (title) {
		int w = GetTextWidth(screen, title) + BORDER_WIDTH*2 + GAP*2;
		int th = GetTextHeight(screen);
		int h = th + BORDER_WIDTH*2;
		int x = (DISCX - w) / 2;

		// ࠬ��
		SetBrushColor(screen, RGB(128, 128, 128));
		SetRop2Mode(screen, R2_COPY);
		FillBox(screen, x, h - BORDER_WIDTH, w, BORDER_WIDTH);
		FillBox(screen, x, 0, BORDER_WIDTH, h - 1);
		FillBox(screen, x + w - BORDER_WIDTH, 0, BORDER_WIDTH, h - 1);

		// ����������
		SetBrushColor(screen, RGB(164, 164, 164));
		SetTextColor(screen, clBlack);
		FillBox(screen, x + BORDER_WIDTH, 0, w - BORDER_WIDTH * 2, h - BORDER_WIDTH);

		SetGCBounds(screen, x + BORDER_WIDTH, BORDER_WIDTH,
				w - BORDER_WIDTH * 2, h - BORDER_WIDTH * 2);
		SetTextColor(screen, RGB(64, 64, 64));
		TextOut(screen, GAP, (h - th) / 2 - BORDER_WIDTH*2, title);
		SetGCBounds(screen, 0, 0, DISCX, DISCY);
	}
}

static void label_draw(GCPtr gc, label_t *l) {
	SetTextColor(gc, clBlack);
	TextOut(gc, l->x, l->y, l->text);
}

void window_draw(window_t *w) {
	ClearGC(w->gc, clSilver);
	draw_title(w->gc, w->title);
	for (list_item_t *li = w->controls.head; li; li = li->next) {
		control_t *c = LIST_ITEM(li, control_t);
		control_draw(c);
	}

	for (list_item_t *li = w->labels.head; li; li = li->next) {
		label_t *l = LIST_ITEM(li, label_t);
		label_draw(w->gc, l);
	}
	for (list_item_t *li = w->idlabels.head; li; li = li->next) {
		label_t *l = LIST_ITEM(li, label_t);
		label_draw(w->gc, l);
	}
}

void window_move_focus(window_t *w, bool next) {
	if (w->controls.count == 0)
		return;

	if (w->focused) {
		control_focus(LIST_ITEM(w->focused, control_t), false);

		w->focused = next ? w->focused->next : w->focused->prev;
		if (w->focused == NULL)
			w->focused = next ? w->controls.head : w->controls.tail;
	} else
		w->focused = w->controls.head;
	control_focus(LIST_ITEM(w->focused, control_t), true);
}

bool window_handle_focus_event(window_t *w, const struct kbd_event *e) {
	if (!e->pressed)
		return false;
	bool next;
	switch (e->key) {
		case KEY_TAB:
			next = (e->shift_state & SHIFT_SHIFT) == 0;
			break;
		case KEY_DOWN:
			next = true;
			break;
		case KEY_UP:
			next = false;
			break;
		default:
			return false;
	}
	window_move_focus(w, next);
	return true;
}

static bool window_process(window_t *w, const struct kbd_event *_e) {
	struct kbd_event e = *_e;
	
	if (e.key == KEY_CAPS && e.pressed && !e.repeated) {
		if (kbd_lang_ex == lng_rus)
			kbd_lang_ex = lng_lat;
		else
			kbd_lang_ex = lng_rus;
	}

	e.ch = kbd_get_char_ex(e.key);

	if (w->focused &&
		(control_handle(LIST_ITEM(w->focused, control_t), &e) ||
				window_handle_focus_event(w, &e)))
		return true;

	if (e.pressed) {
		switch (e.key) {
			case KEY_ESCAPE:
				w->dialog_result = 0;
				return false;
		}
	}

	if (w->dialog_result >= 0)
		return false;
	return true;
}



int window_show_dialog(window_t *w) {

	struct kbd_event e;
	kbd_flush_queue();

	window_draw(w);

	do {
		kbd_get_event(&e);
	} while (window_process(w, &e));

	return 0;	
}

control_t *window_get_control(window_t *w, int id) {
	for (list_item_t *li = w->controls.head; li; li = li->next) {
		control_t *c = LIST_ITEM(li, control_t);
		if (c->id == id) {
			w->idfind = li->next;
			return c;
		}
	}
	return NULL;
}
