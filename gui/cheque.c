#include "kbd.h"
#include "paths.h"
#include "gui/gdi.h"
#include "gui/cheque.h"
#include "kkt/fd/ad.h"

static FontPtr fnt = NULL;
static GCPtr screen = NULL;
extern int kbd_lang_ex;
extern int kbd_get_char_ex(int key);
static int active_button = 0;
static C *current_c = NULL;

int cheque_init(void) {
	fnt = CreateFont(_("fonts/terminal10x18.fnt"), false);
  	screen = CreateGC(0, 0, DISCX, DISCY);
    SetFont(screen, fnt);
	cheque_draw();
	current_c = NULL;
	return 0;
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

int cheque_draw(void) {
	const char *cheque_type[] = { "Приход", "Возврат прихода", "Расход", "Возврат расхода" };
	const char *payout_kind[] = { "Наличными", "Безналичными", "В зачет ранее внесенных средств" };
	int x;
	int y = 2;
	int w, h;
	
	ClearGC(screen, clSilver);
	
	for (list_item_t *i1 = _ad->clist.head; i1; i1 = i1->next) {
		C *c = LIST_ITEM(i1, C);
		char cheque_title[32];
		sprintf(cheque_title, "Чек (%s)", cheque_type[c->t1054 - 1]);
		w = TextWidth(fnt, cheque_title);
		h = fnt->max_height + GAP;
		x = (DISCX - w) / 2;
		fill_rect(x - GAP, y, w + GAP*2, h, 2, clBlack, clSilver);
		SetTextColor(screen, clBlack);
		TextOut(screen, x, y + GAP/2, cheque_title);

		fill_rect(10, y, DISCX - 20, fnt->max_height + GAP, 2, clBlack, 0);

		y += fnt->max_height + GAP + 10;
	}
	
#define BUTTON_WIDTH	100
#define BUTTON_HEIGHT	30
	x = ((DISCX - (BUTTON_WIDTH*2 + GAP)) / 2);
	
	draw_button(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Печать", active_button == 0);
	draw_button(x + BUTTON_WIDTH + GAP, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Отмена", active_button == 1);

	return 0;
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
				active_button = !active_button;
				cheque_draw();
				return 1;
			case KEY_ENTER:
				if (active_button == 0)
					current_c = _ad->clist.count > 0 ? LIST_ITEM(_ad->clist.head, C) : NULL;
				else
					current_c = NULL;
				return 0;
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
