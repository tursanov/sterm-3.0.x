#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "list.h"
#include "sysdefs.h"
#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/controls/controls.h"
#include "gui/controls/window.h"
#include "gui/dialog.h"
#include "gui/newcheque.h"
#include "gui/fa.h"


static void button_action(control_t *c, int cmd) {
	window_set_dialog_result(((window_t *)c->parent.parent), cmd);
}

static void article_action(control_t *c, int cmd) {
}

static void listbox_get_item_text(void *obj, char *text, size_t text_size) {
}

int newcheque_execute() {
	LIST_INIT(articles, NULL);
	window_t *win = window_create(NULL, "Новый чек (Esc - выход)");
	GCPtr screen = window_get_gc(win);

#define GAP 10
#define YGAP	2
#define TEXT_START		GAP
#define	CONTROLS_START	300 
#define CONTROLS_TOP	30
#define BUTTON_WIDTH	100
#define BUTTON_HEIGHT	30

	int x = CONTROLS_START;
	int y = CONTROLS_TOP;
	int w = DISCX - x - GAP;
	int th = GetTextHeight(screen);
	int h = th + 4;

	const char *pay_type[] = {
		"Приход",
		"Возврат прихода",
		"Расход",
		"Возврат расхода"
	};
	const char *pay_kind[] = {
		"Наличные",
		"Безналичные"
	};

	window_add_label(win, TEXT_START, y, "Признак расчета:");
	window_add_control(win, 
			simple_combobox_create(1054, screen, x, y, w, h, pay_type, ASIZE(pay_type), 0));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y, "Вид оплаты:");
	window_add_control(win, 
			simple_combobox_create(9998, screen, x, y, w, h, pay_kind, ASIZE(pay_kind), 0));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y, "Кассир:");
	window_add_control(win, 
			edit_create(1021, screen, x, y, w, h, cashier_get_name(), EDIT_INPUT_TYPE_TEXT, 64));
	y += h + YGAP;

	
	window_add_label(win, TEXT_START, y, "Должность:");
	window_add_control(win,
			edit_create(9999, screen, x, y, w, h, cashier_get_post(), EDIT_INPUT_TYPE_TEXT, 64));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y, "ИНН кассира:");
	window_add_control(win,
			edit_create(1203, screen, x, y, w, h, cashier_get_inn(), EDIT_INPUT_TYPE_TEXT, 12));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y, "Тел. или e-mail покупателя:");
	window_add_control(win,
			edit_create(1008, screen, x, y, w, h, NULL, EDIT_INPUT_TYPE_TEXT, 12));
	y += h + YGAP + th + 12;

/*	window_add_control(win,
			button_create(2, screen, DISCX - (BUTTON_WIDTH + GAP)*2, y, 
				BUTTON_WIDTH, BUTTON_HEIGHT - 4, 1, "Добавить", article_action));
	window_add_control(win,
			button_create(3, screen, DISCX - (BUTTON_WIDTH + GAP)*1, y, 
				BUTTON_WIDTH, BUTTON_HEIGHT - 4, 2, "Удалить", article_action));*/

	//y += BUTTON_HEIGHT;
	window_add_label(win, TEXT_START, y - th - 4, 
			"Предметы расчета (\"+\" добавить, \"-\" удалить):");

	const char *text = "ИТОГО: 0.00 руб";

	window_add_label_with_id(win, 1, DISCX - GAP - GetTextWidth(screen, text), y - th - 4, text);

	window_add_control(win,
			listbox_create(9998, screen, GAP, y, DISCX - GAP*2, DISCY - y - BUTTON_HEIGHT - GAP*2,
				&articles, listbox_get_item_text, 0));

	window_add_control(win, 
			button_create(1, screen, DISCX - (BUTTON_WIDTH + GAP)*2, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 1, "Печать", button_action));
	window_add_control(win, 
			button_create(0, screen, DISCX - (BUTTON_WIDTH + GAP)*1, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 1, "Отмена", button_action));

	int result = window_show_dialog(win);
	window_destroy(win);

	return result;
}

/*
static void draw_divider(int x, int y, const char *text) {
	SetTextColor(screen, clGray);
	TextOut(screen, x, y, text);
	x += GetTextWidth(screen, text) + 2;
	y += GetTextHeight(screen) / 2 - 1;
	int w = DISCX - x - GAP;
	SetBrushColor(screen, clGray);
	FillBox(screen, x, y, w, 2);
}


static void draw_total_sum(int x, int y, uint64_t sum) {
	char text[32];

	sprintf(text, "ИТОГО: %.1lld.%.2lld", sum / 100, sum % 100);

	SetTextColor(screen, clBlack);
	TextOut(screen, x, y, text);
}

void newcheque_draw() {
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
	ClearGC(screen, clSilver);
	draw_title(screen, fnt, "Новый чек (Esc - выход)");
	cpairs_draw();

	draw_divider(GAP, 176, "Предметы расчета");
	draw_total_sum(GAP*2, 176 + GetTextHeight(screen) + 2, 0); 
}*/
