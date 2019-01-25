#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "list.h"
#include "sysdefs.h"
#include "kbd.h"
#include "kkt/fd/tlv.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/controls/controls.h"
#include "gui/controls/window.h"
#include "gui/dialog.h"
#include "gui/newcheque.h"
#include "gui/fa.h"
#include "gui/references/article.h"

#define GAP 10
#define YGAP	2
#define TEXT_START		GAP
#define	CONTROLS_START	300 
#define CONTROLS_TOP	30
#define BUTTON_WIDTH	100
#define BUTTON_HEIGHT	30

typedef struct {
	article_t *article;
	ffd_fvln_t count;
} cheque_article_t;

LIST_INIT(cheque_articles, NULL);

static void button_action(control_t *c, int cmd) {
	window_set_dialog_result(((window_t *)c->parent.parent), cmd);
}

extern int article_get_text(void *obj, int index, char *text, size_t text_size);


extern void cheque_article_get_text(void *obj, char *text, size_t text_size) {
//	cheque_article_t *a = (cheque_article_t *)obj;
	/*char strcountv[16];
	char strcount[256];
	int length = sprintf(strcountv, "%lld", a->count.value);*/

	//snprintf(text, text_size, "%s %sX%1lld.%.2lld", a->name, 
}

bool article_new_handle(window_t *w, const struct kbd_event *e) {
/*	switch (e->key) {
		case KEY_ENTER:
		case KEY_NUMENTER:
			window_set_dialog_result(w, 1);
			break;
	}*/
	return true;
}

static void article_new(window_t *parent) {
	window_t *win = window_create(NULL, "Выберите товар/работу/услугу", article_new_handle);
	GCPtr screen = window_get_gc(win);
	listview_column_t columns[] = {
		{ "\xfc", 30 },
		{ "Наименование", 150 },
		{ "СНО", 50 },
		{ "Способ расчета", 150 },
		{ "Цена за ед.", 120 },
		{ "Ставка НДС", 120 },
		{ "Признак агента", 158 },
	};

	int x = CONTROLS_START;
	int y = CONTROLS_TOP;
	int w = DISCX - x - GAP;
	int th = GetTextHeight(screen);
	int h = DISCY - 150;

	window_add_label(win, TEXT_START, y, "Список товаров/работ/услуг:");
	y += th + 2;


	window_add_control(win, 
			listview_create(1059, screen, TEXT_START, y, DISCX - GAP * 2, h, columns, ASIZE(columns),
				&articles, (listview_get_item_text_func_t)article_get_text, 0));
	y += h + GAP * 2;

	window_add_label(win, TEXT_START, y + 4, "Кол-во:");
	window_add_control(win,
			edit_create(1023, screen, TEXT_START + 100, y, w, th + 8, NULL, EDIT_INPUT_TYPE_DOUBLE, 16));


	x = (DISCX - (BUTTON_WIDTH + GAP)*2 - GAP) / 2;
	window_add_control(win, 
			button_create(1, screen, x, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 1, "Добавить", button_action));
	window_add_control(win, 
			button_create(0, screen, x + BUTTON_WIDTH + GAP,
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 1, "Отмена", button_action));

	int result;
	while ((result = window_show_dialog(win)) == 1) {
		article_t *a = window_get_ptr_data(win, 1059, 1);
		if (a == NULL) {
			message_box("Ошибка", "Не выбран товар/работа/услуга", dlg_yes, 0, al_center);
			window_set_focus(win, 1059);
		}
		data_t data;
		window_get_data(win, 1023, 0, &data);
		if (data.size == 0) {
			message_box("Ошибка", "Не указано кол-во", dlg_yes, 0, al_center);
			window_set_focus(win, 1023);
		}
	}

	window_destroy(win);
	window_draw(parent);
}

static void article_delete(window_t *parent) {
}

static void listbox_get_item_text(void *obj, char *text, size_t text_size) {
	cheque_article_t *ca = (cheque_article_t *)obj;
	snprintf(text, text_size, "%.1lld.%.2lldX%lld %s",
			ca->article->price_per_unit / 100, ca->article->price_per_unit % 100,
			ca->count.value, ca->article->name);
}

bool newcheque_process(window_t *w, const struct kbd_event *e) {
	switch (e->key) {
		case KEY_NUMPLUS:
		case KEY_PLUS:
			article_new(w);
			break;
		case KEY_MINUS:
		case KEY_NUMMINUS:
			article_delete(w);
			break;
	}
	return true;
}

int newcheque_execute() {
	window_t *win = window_create(NULL, "Новый чек (Esc - выход)", newcheque_process);
	GCPtr screen = window_get_gc(win);

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

	window_add_label(win, TEXT_START, y - th - 4, 
			"Предметы расчета (\"+\" добавить, \"-\" удалить):");

	const char *text = "ИТОГО: 0.00 руб";

	window_add_label_with_id(win, 1, DISCX - GAP - GetTextWidth(screen, text), y - th - 4, text);

	window_add_control(win,
			listbox_create(9998, screen, GAP, y, DISCX - GAP*2, DISCY - y - BUTTON_HEIGHT - GAP*2,
				&cheque_articles, listbox_get_item_text, 0));

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
