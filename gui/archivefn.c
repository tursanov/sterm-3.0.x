#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "list.h"
#include "sysdefs.h"
#include "kbd.h"
#include "kkt/fd/tlv.h"
#include "kkt/fd/ad.h"
#include "kkt/kkt.h"
#include "paths.h"
#include "serialize.h"
#include "gui/gdi.h"
#include "gui/controls/controls.h"
#include "gui/controls/window.h"
#include "gui/dialog.h"
#include "gui/archivefn.h"
#include "gui/fa.h"

#define GAP 10
#define YGAP	2
#define TEXT_START		GAP
#define	CONTROLS_START	300 
#define CONTROLS_TOP	30
#define BUTTON_WIDTH	100
#define BUTTON_HEIGHT	30

static int op_kind;
static int res_out;
static uint32_t doc_no;
static list_t output = { NULL, NULL, 0, NULL };

static bool archivefn_get_archive_doc() {
	return true;
}

static bool archivefn_get_doc() {
	return true;
}

static bool archivefn_get_data() {
	if (op_kind == 0)
		return archivefn_get_archive_doc();
	return archivefn_get_doc();
}

static void button_action(control_t *c, int cmd) {
	window_set_dialog_result(((window_t *)c->parent.parent), cmd);
}

static void listbox_get_item_text(void *obj, char *text, size_t text_size) {
	text[0] = 0;
}

int archivefn_execute() {
	window_t *win = window_create(NULL, 
			"Просмотр/печать документов из ФН (архива ФН) (Esc - выход)", NULL);
	GCPtr screen = window_get_gc(win);

	int x = CONTROLS_START;
	int y = CONTROLS_TOP;
	int w = DISCX - x - GAP;
	int th = GetTextHeight(screen);
	int h = th + 8;

	const char *str_op_kind[] = {
		"Документ из архива ФН",	
		"Документ из ФН",
	};
	const char *str_res_out[] = {
		"На экран",
		"На экран и на печать"
	};
	char str_doc_no[16];

	if (doc_no != 0)
		sprintf(str_doc_no, "%d", doc_no);

	window_add_label(win, TEXT_START, y, align_left, "Вид операции:");
	window_add_control(win, 
			simple_combobox_create(9999, screen, x, y, w, h, 
				str_op_kind, ASIZE(str_op_kind), op_kind));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y, align_left, "Вывод результата:");
	window_add_control(win, 
			simple_combobox_create(9998, screen, x, y, w, h, 
				str_res_out, ASIZE(str_res_out), res_out));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y, align_left, "Номер документа:");
	window_add_control(win, 
			edit_create(1040, screen, x, y, w, h, doc_no == 0 ? NULL : str_doc_no,
				EDIT_INPUT_TYPE_TEXT, 10));
	y += h + YGAP + th + 12;

	window_add_label(win, TEXT_START, y - th - 4, align_left, "Данные документа:");

	window_add_control(win,
			listbox_create(9997, screen, GAP, y, DISCX - GAP*2, DISCY - y - BUTTON_HEIGHT - GAP*2,
				&output, listbox_get_item_text, 0));

	window_add_control(win, 
			button_create(1, screen, DISCX - (BUTTON_WIDTH + GAP)*2, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 1, "Выполнить", button_action));
	window_add_control(win, 
			button_create(0, screen, DISCX - (BUTTON_WIDTH + GAP)*1, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 0, "Закрыть", button_action));

	int result;
	while (true) {
		result = window_show_dialog(win);
		data_t data;

		op_kind = window_get_int_data(win, 9999, 0, 0);
		res_out = window_get_int_data(win, 9998, 0, 0);

		if (result == 0)
			break;

		window_get_data(win, 1040, 0, &data);

		if (data.size == 0) {
			window_show_error(win, 1040, "Поле \"Номер документа\" не заполнено");
			continue;
		}
		doc_no = atoi(data.data);
		if (doc_no == 0) {
			window_show_error(win, 1040, "Данное значение поля \"Номер документа\" недопустимо");
			continue;
		}

		if (!archivefn_get_data())
			continue;
	}

	window_destroy(win);

	return result;
}
