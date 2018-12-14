#include "kbd.h"
#include <sys/param.h>
#include "paths.h"
#include "gui/gdi.h"
#include "gui/cheque_docs.h"
#include "gui/forms.h"
#include "gui/dialog.h"
#include "kkt/fd/ad.h"
#include <string.h>
#include <stdlib.h>

static FontPtr fnt = NULL;
static GCPtr screen = NULL;
static int active_button = 0;
static int active_item = 0;

int cheque_docs_init(void) {
	if (fnt == NULL)
		fnt = CreateFont(_("fonts/fixedsys8x16.fnt"), false);
	if (screen == NULL)
	  	screen = CreateGC(0, 0, DISCX, DISCY);
    SetFont(screen, fnt);

	cheque_docs_draw();

	return 0;
}

void cheque_docs_release(void) {
	if (fnt) {
		DeleteFont(fnt);
		fnt = NULL;
	}
	if (screen) {
		DeleteGC(screen);
		screen = NULL;
	}
}

#define GAP 10
#define LIST_WIDTH	300

int cheque_docs_draw() {
	int x, y;
	char text[512];
	ClearGC(screen, clSilver);
	draw_title(screen, fnt, "Удаление документа из чека");
	
	SetTextColor(screen, clBlack);

	sprintf(text, _ad->docs.count > 0 ? "Выберите документ из списка и нажмите Del" :
		"Нет документов в корзине ФП");
	x = (DISCX - TextWidth(fnt, text)) / 2;
	y = 30;
	TextOut(screen, x, y, text);
	y += fnt->max_height + GAP;

	if (_ad->docs.count > 0) {
		x = (DISCX - LIST_WIDTH) / 2;
	
		fill_rect(screen, x, y, LIST_WIDTH, ((fnt->max_height + GAP) * _ad->docs.count) + 4, 2,
			clGray, clRopnetBrown);
		
		y = 30 + fnt->max_height + GAP + 2;
		
		for (int i = 0; i < _ad->docs.count; i++) {
			if (active_item == i) {
				SetBrushColor(screen, clNavy);
				SetTextColor(screen, clWhite);
				FillBox(screen, x + 2, y, LIST_WIDTH - 4, fnt->max_height + GAP);
			} else {
				SetTextColor(screen, clRopnetDarkBrown);
			}
			sprintf(text, "%.14lld", _ad->docs.values[i]);
			TextOut(screen, x + GAP, y + GAP / 2, text);
			y += fnt->max_height + GAP;
		}
		
		y += GAP;
	}
	
	
#define BUTTON_WIDTH	100
#define BUTTON_HEIGHT	30
	x = (DISCX - BUTTON_WIDTH) / 2;
	draw_button(screen, fnt, x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Закрыть", true);

	return 0;
}

static void set_active_item(int n) {
	int x = (DISCX - LIST_WIDTH) / 2;
	char text[512];

	for (int i = 0; i < 2; i++) {
		int y = 30 + (fnt->max_height + GAP) * (active_item + 1)  + 2;
		if (i == 0 && active_item >= 0 && active_item < _ad->docs.count) {
			SetTextColor(screen, clRopnetDarkBrown);
			SetBrushColor(screen, clRopnetBrown);
		} else {
			SetBrushColor(screen, clNavy);
			SetTextColor(screen, clWhite);
		}
		if (n < 0 || n >= _ad->docs.count)
			break;
		FillBox(screen, x + 2, y, LIST_WIDTH - 4, fnt->max_height + GAP);
		sprintf(text, "%.14lld", _ad->docs.values[active_item]);
		TextOut(screen, x + GAP, y + GAP / 2, text);
		active_item = n;
	}
}

static void del_item() {
	char text[512];
	int64_t n = _ad->docs.values[active_item];
	sprintf(text, "Документ с номером %.14lld будет удален.\nПродолжить?", n);

	if (message_box("Предупреждение", text, dlg_yes_no, 0, al_center) == DLG_BTN_YES)
		AD_delete_doc(n);
	cheque_docs_draw();
}

static bool cheque_docs_process(const struct kbd_event *e) {
	if (e->pressed) {
		switch (e->key) {
			case KEY_ESCAPE:
			case KEY_ENTER:
			case KEY_NUMENTER:
				return 0;
			case KEY_TAB:
			case KEY_DOWN:
				if (active_item < _ad->docs.count - 1)
					set_active_item(active_item + 1);
				return 1;
			case KEY_UP:
				if (active_item > 0)
					set_active_item(active_item - 1);
				return 1;
			case KEY_DEL:
				if (active_item >= 0 && active_item < _ad->docs.count)
					del_item();
				return 1;
		}
	}

	return 1;
}

void cheque_docs_execute(void) {
	struct kbd_event e;
	int ret;

	kbd_flush_queue();

	do {
		kbd_get_event(&e);
	} while ((ret = cheque_docs_process(&e)) > 0);
}
