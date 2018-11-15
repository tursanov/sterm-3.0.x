#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/cheque.h"
#include "gui/forms.h"
#include "kkt/fd/ad.h"
#include <string.h>
#include <stdlib.h>

static FontPtr fnt = NULL;
static GCPtr screen = NULL;
extern int kbd_lang_ex;
extern int kbd_get_char_ex(int key);
static int active_button = 0;
static C *current_c = NULL;
static list_item_t *active_item = NULL;
static int active_item_child = 0;

int cheque_init(void) {
	if (fnt == NULL)
		fnt = CreateFont(_("fonts/fixedsys8x16.fnt"), false);
	if (screen == NULL)
	  	screen = CreateGC(0, 0, DISCX, DISCY);
    SetFont(screen, fnt);
	cheque_draw();
	current_c = NULL;
	return 0;
}

void cheque_release(void) {
	if (fnt) {
		DeleteFont(fnt);
		fnt = NULL;
	}
	if (screen) {
		DeleteGC(screen);
		screen = NULL;
	}
}

void fill_rect(int x, int y, int width, int height, int border_width,
		Color border_color, int bg_color) {
	// рамка
	SetBrushColor(screen, border_color);
	SetRop2Mode(screen, R2_COPY);
	FillBox(screen, x, y, width, border_width);
	FillBox(screen, x, y + height - border_width, width, border_width);
	FillBox(screen, x, y + border_width, border_width, height - border_width - 1);
	FillBox(screen, x + width - border_width, y + border_width, 
		border_width, height - border_width - 1);

	// заполнение
	if (bg_color > 0) {
		SetBrushColor(screen, bg_color);
		FillBox(screen, x + border_width, y + border_width,
			width - border_width * 2, height - border_width * 2);
	}
}
#define GAP 10

void draw_button(int x, int y, int width, int height, const char *text, bool focused) {
	Color borderColor;
	Color bgColor;
	Color fgColor;

	if (focused) {
		borderColor = clRopnetDarkBrown;
		bgColor = clRopnetBrown;
		fgColor = clBlack;
	} else {
		borderColor = RGB(184, 184, 184);
		bgColor = RGB(200, 200, 200);
		fgColor = RGB(32, 32, 32);
	}

	fill_rect(x, y, width, height, 2, borderColor, bgColor);
	int tw = TextWidth(fnt, text);

	x += (width - tw) / 2;
	y += (height - fnt->max_height)/2;
	SetTextColor(screen, fgColor);
	TextOut(screen, x, y, text);
}

#define IR_COLOR	clGray

static int email_or_phone_draw(C *c, int start_y) {
	int y = start_y + fnt->max_height*4 + GAP + 4;
	char email_or_phone[128];
	const char *title = "Тел. или e-mail покупателя:    ";
	sprintf(email_or_phone, "%s", c->pe ? c->pe : "не указан");
	int tw = TextWidth(fnt, title);
	int tw2 = TextWidth(fnt, email_or_phone);
	Color selectedColor = (active_item && LIST_ITEM(active_item, C) == c &&
			active_item_child == 0) ? clRopnetDarkBrown : clSilver;

	SetTextColor(screen, c->pe ? clBlack : IR_COLOR);
	TextOut(screen, GAP*2, y, title);
	TextOut(screen, GAP*2 + tw, y, email_or_phone);

	fill_rect(GAP*2 + tw - 2, y - 2, tw2 + 4, fnt->max_height + 4, 2, selectedColor, 0);

	y += fnt->max_height + GAP;

	return y;
}

static int cheque_draw_cheque(C *c, int start_y) {
	const char *cheque_type[] = { "Приход", "Возврат прихода", "Расход", "Возврат расхода" };
	const char *payout_kind[] = { "Наличными:", "Безналичными:", "В зачет ранее внесенных средств:", "ИТОГО:" };
	int64_t s[4] = { c->sum.n, c->sum.e, c->sum.p, c->sum.a };
	char ss[4][32];
	char cheque_title[32];
	int sw;
	int x, w, h;
	int y = start_y;

	sprintf(cheque_title, "Чек (%s)", cheque_type[c->t1054 - 1]);
	w = TextWidth(fnt, cheque_title);
	h = fnt->max_height + GAP;
	x = (DISCX - w - GAP*2);
	fill_rect(x - GAP, y, w + GAP*2, h, 2, clGray, clSilver);
	SetTextColor(screen, clBlack);
	TextOut(screen, x, y + GAP/2, cheque_title);
	y += GAP;

	w = 0;
	sw = 0;
	for (int i = 0; i < ASIZE(payout_kind); i++) {
		sprintf(ss[i], "%.1lld.%.2lld", s[i] / 100, s[i] % 100);
		int tw = TextWidth(fnt, payout_kind[i]);
		if (tw > w)
			w = tw;
		tw = TextWidth(fnt, ss[i]);
		if (tw > sw)
			sw = tw;
	}

	for (int i = 0; i < ASIZE(payout_kind); i++) {
		int tw = TextWidth(fnt, ss[i]);
		SetTextColor(screen, s[i] > 0 ? clBlack : IR_COLOR);
		TextOut(screen, GAP*2, y, payout_kind[i]);
		TextOut(screen, GAP*3 + w + sw - tw, y, ss[i]);
		y += fnt->max_height;
		if (i == 2)
			y += 4;
	}

	y = email_or_phone_draw(c, start_y);

	fill_rect(10, start_y, DISCX - 20, y - start_y, 2, clGray, 0);
	y += GAP;

	return y;
}

int cheque_draw() {
	ClearGC(screen, clSilver);

	int x;
	int y = 0;
	for (list_item_t *i1 = _ad->clist.head; i1; i1 = i1->next) {
		C *c = LIST_ITEM(i1, C);
		y = cheque_draw_cheque(c, y);
	}

#define BUTTON_WIDTH	100
#define BUTTON_HEIGHT	30
	x = ((DISCX - (BUTTON_WIDTH*2 + GAP)) / 2);

	draw_button(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Печать", active_button == 0);
	draw_button(x + BUTTON_WIDTH + GAP, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Отмена", active_button == 1);

	return 0;
}

static void next_focus() {
	if (active_button == 0)
		active_button = 1;
	else if (active_button == 1) {
		if (_ad->clist.head) {
			active_item = _ad->clist.head;
			active_item_child = 0;
			active_button = -1;
		} else {
			active_button = 1;
		}
	} else {
		active_item = active_item->next;
		active_item_child = 0;
		if (active_item == NULL)
			active_button = 0;
	}
	cheque_draw();
}

static void select_phone_or_email() {
	C *c = LIST_ITEM(active_item, C);
	form_t *form = NULL;
	BEGIN_FORM(form, "Ввод тедефона или e-mail получателя чека")
		FORM_ITEM_EDIT_TEXT(1008, "Тел. или e-mail:", c->pe, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_BUTTON(1, "ОК", NULL)
		FORM_ITEM_BUTTON(0, "Отмена", NULL)
	END_FORM()

	kbd_lang_ex = lng_lat;
	if (form_execute(form) == 1) {
		form_data_t data;
		form_get_data(form, 1008, 1, &data);
		
		if (c->pe)
		   free(c->pe);
		if (data.size > 0)
			c->pe = strdup((char *)data.data);
		else
			c->pe = NULL;
		AD_save();
	}
	cheque_draw();
	form_destroy(form);
}

static bool cheque_process(const struct kbd_event *_e) {
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
				current_c = NULL;
				return 0;
			case KEY_TAB:
				next_focus();
				return 1;
			case KEY_ENTER:
				if (active_button == 0) {
					current_c = LIST_ITEM(_ad->clist.head, C);
					return 0;
				} else if (active_button == 1) {
					current_c = NULL;
					return 0;
				} else if (active_item_child == 0) {
					select_phone_or_email();
				}
		}
	}

	return 1;
}


C* cheque_execute(void) {
	struct kbd_event e;
	int ret;

	kbd_flush_queue();

	do {
		kbd_get_event(&e);
	} while ((ret = cheque_process(&e)) > 0);

	return current_c;
}
