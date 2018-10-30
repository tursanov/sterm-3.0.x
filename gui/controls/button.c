typedef struct button_t {
	control_t control;
	char *text;
	bool pressed;
	int result;
	form_action_t action;
} button_t;

void button_destroy(button_t *button);
void button_draw(button_t *button);
bool button_focus(button_t *button, bool focus);
bool button_handle(button_t *button, const struct kbd_event *e);

button_t* button_create(int x, int y, int width, int height, form_t *form, form_item_info_t *info)
{
    button_t *button = (button_t *)malloc(sizeof(button_t));
    control_api_t api = {
		(void (*)(struct control_t *))button_destroy,
		(void (*)(struct control_t *))button_draw,
		(bool (*)(struct control_t *, bool))button_focus,
		(bool (*)(struct control_t *, const struct kbd_event *))button_handle,
		NULL,
    };

    control_init(&button->control, x, y, width, height, form, &api);

	if (info->text != NULL)
	    button->text = strdup(info->text);

	button->pressed = false;
	button->result = info->id;
	button->action = info->action;

    return button;
}

void button_destroy(button_t *button) {
	if (button->text != NULL)
		free(button->text);
	free(button);
}

void button_draw(button_t *button) {
	Color borderColor;
	Color bgColor;
	Color fgColor;

	if (button->control.focused) {
		borderColor = clRopnetDarkBrown;
		bgColor = clRopnetBrown;
		fgColor = clBlack;
	} else {
		borderColor = RGB(184, 184, 184);
		bgColor = RGB(200, 200, 200);
		fgColor = RGB(32, 32, 32);
	}

	// рамка
	SetBrushColor(screen, borderColor);
	SetRop2Mode(screen, R2_COPY);
	FillBox(screen, button->control.x, button->control.y, button->control.width, BORDER_WIDTH);
	FillBox(screen, button->control.x, button->control.y + button->control.height - BORDER_WIDTH, button->control.width, BORDER_WIDTH);
	FillBox(screen, button->control.x, button->control.y + BORDER_WIDTH, BORDER_WIDTH, button->control.height - BORDER_WIDTH - 1);
	FillBox(screen, button->control.x + button->control.width - BORDER_WIDTH, button->control.y + BORDER_WIDTH, 
		BORDER_WIDTH, button->control.height - BORDER_WIDTH - 1);

	// заполнение
	SetBrushColor(screen, bgColor);
	FillBox(screen, button->control.x + BORDER_WIDTH, button->control.y + BORDER_WIDTH, 
		button->control.width - BORDER_WIDTH * 2, button->control.height - BORDER_WIDTH * 2);

	if (button->text) {
		SetGCBounds(screen, button->control.x + BORDER_WIDTH, button->control.y + BORDER_WIDTH,
			button->control.width - BORDER_WIDTH * 2, button->control.height - BORDER_WIDTH * 2);
		int w = TextWidth(form_fnt, button->text);
		int x = (button->control.width - w) / 2;
		int y = (button->control.height - form_fnt->max_height) / 2 - BORDER_WIDTH;
		SetTextColor(screen, fgColor);

		if (button->pressed) {
			x += 1;
			y += 1;
		}

		TextOut(screen, x, y, button->text);
	}
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

bool button_focus(button_t *button, bool focus) {
	button->control.focused = focus;
	button_draw(button);
	return true;
}

bool button_handle(button_t *button, const struct kbd_event *e) {
	switch (e->key) {
	case KEY_ENTER:
	case KEY_SPACE:
		if (e->pressed && !e->repeated) {
			button->pressed = true;
			button_draw(button);
			button->pressed = false;
			if (!button->action || button->action(button->control.form))
				button->control.form->result = button->result;
			kbd_flush_queue();
		}
		break;
	}
	return true;
}

