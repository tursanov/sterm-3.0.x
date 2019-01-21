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
#include "gui/newcheque.h"

static FontPtr fnt = NULL;
static GCPtr screen = NULL;
static int result;

extern void draw_title(GCPtr screen, FontPtr fnt, const char *title);
extern void fill_rect(GCPtr screen, int x, int y, int width, int height, int border_width,
		Color border_color, int bg_color);

extern int kbd_lang_ex;
extern int kbd_get_char_ex(int key);

static bool newcheque_process(const struct kbd_event *_e) {
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
		}
	}

	return true;
}

int newcheque_execute() {
	struct kbd_event e;

	fnt = CreateFont(_("fonts/terminal10x18.fnt"), false);
   	screen = CreateGC(0, 0, DISCX, DISCY);
    SetFont(screen, fnt);

	newcheque_draw();

	kbd_flush_queue();
	do {
		kbd_get_event(&e);
	} while (newcheque_process(&e));

	DeleteFont(fnt);
	DeleteGC(screen);

	return result;
}

void newcheque_draw() {
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
	ClearGC(screen, clSilver);
	draw_title(screen, fnt, "Новый чек (Esc - выход)");
}
