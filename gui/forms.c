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

#define	GAP	10
#define DEFAULT_CAPACITY    256
#define BORDER_WIDTH		2

#include "controls/control.h"
#include "controls/lang.c"

typedef struct form_item_t {
	form_item_type_t type;
	int id;
	char *name;
	size_t name_len;
	int name_x;
	int name_y;
	control_t *control;
} form_item_t;

struct form_t {
	char *name;
	int name_width;
	size_t item_count;
	form_item_t *items;
	int active_index;
	int result;
};

static int ref_count = 0;
static FontPtr form_fnt = NULL;
static GCPtr screen = NULL;

#include "controls/edit.c"
#include "controls/button.c"
#include "controls/control.c"

static void form_draw_title(form_t *form);

form_t* form_create(const char *name, form_item_info_t items[], size_t item_count) {
	if (ref_count++ == 0) {
	    form_fnt = CreateFont(_("fonts/terminal10x18.fnt"), false);
    	screen = CreateGC(0, 0, DISCX, DISCY);
	    SetFont(screen, form_fnt);
    }
	ClearGC(screen, clSilver);
	form_t *form = (form_t *)malloc(sizeof(form_t));
	if (name) {
		form->name = strdup(name);
		form_draw_title(form);
	}
	form->result = -1;

	form->item_count = item_count;
	form->items = (form_item_t *)malloc(sizeof(form_item_t) * form->item_count);
	form->active_index = 0;

	size_t max_name_len = 0;
    for (int i = 0; i < item_count; i++) {
    	form_item_info_t *info = &items[i];
    	form_item_t *item = &form->items[i];

   		item->type = info->type;

		if (info->name != NULL) {
	   		item->name = strdup(info->name);
	  		item->name_len = strlen(info->name);
	   		if (item->name_len > max_name_len)
				max_name_len = item->name_len;
		} else {
			item->name = NULL;
			item->name_len = 0;
		}
	}

	int hw = (DISCX * 2) / 3;
	int max_name_w = (form_fnt->max_width * max_name_len) + GAP*2;

	int control_x = (max_name_w > hw ? hw : max_name_w);
	int y = form_fnt->max_height + GAP*2;
	int x = GAP*2;
   	int w = 200;
	int h;

	form->name_width = control_x - GAP;
	form_item_info_t *prev = NULL;

    for (int i = 0; i < item_count; i++) {
    	form_item_info_t *info = &items[i];
    	form_item_t *item = &form->items[i];

    	item->id = info->id;

    	if (prev != NULL) {
    		if (prev->name != NULL) {
				y += h + GAP;
				x = GAP * 2;
    		}
    	}

		if (info->name) {
			item->name_x = GAP*2;
			item->name_y = y;
			TextOut(screen, GAP*2, y, info->name);
			x = control_x + GAP;
			w = DISCX - control_x - GAP*3;
			h = form_fnt->max_height + BORDER_WIDTH*2 + 2;
		} else {
			size_t count = 1;
			if (!prev || prev->name != NULL) {
				w = 30;
				for (int j = i + 1; j < item_count; j++, count++) {
					if (items[j].name != NULL)
						break;
					int tw = TextWidth(form_fnt, items[j].text) + GAP*2;
					if (tw > w)
						w = tw;
				}
				x = (DISCX - ((w + GAP) * count)) / 2;
			}

			h = form_fnt->max_height + GAP;
		}

		item->control = control_create(x, y, w, h, form, info);
		if (i == form->active_index)
			control_focus(item->control, true);
		else
			control_draw(item->control);

		x += w + GAP*2;

/*		if (info->name != NULL)
			y += h + GAP;*/

		prev = info;
    }

	return form;
}

static void form_draw_title(form_t *form) {
	if (form->name) {
		int w = TextWidth(form_fnt, form->name) + BORDER_WIDTH*2 + GAP*2;
		int h = form_fnt->max_height + BORDER_WIDTH*2;
		int x = (DISCX - w) / 2;

		// рамка
		SetBrushColor(screen, RGB(128, 128, 128));
		SetRop2Mode(screen, R2_COPY);
		FillBox(screen, x, h - BORDER_WIDTH, w, BORDER_WIDTH);
		FillBox(screen, x, 0, BORDER_WIDTH, h - 1);
		FillBox(screen, x + w - BORDER_WIDTH, 0, BORDER_WIDTH, h - 1);

		// заполнение
		SetBrushColor(screen, RGB(164, 164, 164));
		SetTextColor(screen, clBlack);
		FillBox(screen, x + BORDER_WIDTH, 0, w - BORDER_WIDTH * 2, h - BORDER_WIDTH);

		SetGCBounds(screen, x + BORDER_WIDTH, BORDER_WIDTH,
				w - BORDER_WIDTH * 2, h - BORDER_WIDTH * 2);
		SetTextColor(screen, RGB(64, 64, 64));
		TextOut(screen, GAP, (h - form_fnt->max_height) / 2 - BORDER_WIDTH*2, form->name);
		SetGCBounds(screen, 0, 0, DISCX, DISCY);
	}
}

void form_draw(form_t *form) {
	ClearGC(screen, clSilver);
	form_draw_title(form);
    for (int i = 0; i < form->item_count; i++) {
    	form_item_t *item = &form->items[i];
		if (item->name)
			TextOut(screen, item->name_x, item->name_y, item->name);
		control_draw(item->control);
	}
}

void form_destroy(form_t *form) {
	if (form != NULL) {
	    for (int i = 0; i < form->item_count; i++) {
	    	form_item_t *item = &form->items[i];
	    	if (item->name)
		   		free(item->name);
	   		control_destroy(item->control);
		}
		free(form->items);
		free(form->name);
		free(form);

		if (--ref_count == 0) {
		    DeleteFont(form_fnt);
	    	DeleteGC(screen);
		}
	}
}

static bool form_process(form_t *form, const struct kbd_event *_e) {
	control_t *active_control = form->items[form->active_index].control;
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
				form->result = 0;
				return false;
			case KEY_TAB:
				control_focus(active_control, false);
				if (e.shift_state & SHIFT_SHIFT) {
					form->active_index--;
					if (form->active_index < 0)
						form->active_index = form->item_count - 1;
				} else {
					form->active_index++;
					if (form->active_index >= form->item_count)
						form->active_index = 0;
				}
				active_control = form->items[form->active_index].control;
				control_focus(active_control, true);
				break;
		}
	}

	control_handle(active_control, &e);

	if (form->result >= 0)
		return false;

	return true;
}

int form_execute(form_t *form)
{
	struct kbd_event e;
	int ret;

	kbd_flush_queue();

	if (form == NULL)
		return -1;
	do {
		kbd_get_event(&e);
	} while ((ret = form_process(form, &e)) > 0);

	int result = form->result;
	form->result = -1;

	return result;
}

bool form_get_text(form_t *form, int id, form_text_t *text, bool trim) {
    for (int i = 0; i < form->item_count; i++) {
    	form_item_t *item = &form->items[i];
    	if (item->id == id)
    		return control_get_text(item->control, text, trim);
    }
    return false;
}
