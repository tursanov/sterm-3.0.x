typedef struct listbox_t {
	control_t control;
	char **items;
	size_t item_count;
	bool expanded;
	int dropdown_y;
	int dropdown_height;
	int selected_index;
	int tmp_selected_index;
	edit_t *edit;
} listbox_t;

void listbox_destroy(listbox_t *listbox);
void listbox_draw(listbox_t *listbox);
bool listbox_focus(listbox_t *listbox, bool focus);
bool listbox_handle(listbox_t *listbox, const struct kbd_event *e);
bool listbox_get_data(listbox_t *listbox, int what, form_data_t *data);
bool listbox_set_data(listbox_t *listbox, int what, const void *data, size_t data_len);

listbox_t* listbox_create(int x, int y, int width, int height, form_t *form,
	form_item_info_t *info)
{
    listbox_t *listbox = (listbox_t *)malloc(sizeof(listbox_t));
    control_api_t api = {
		(void (*)(struct control_t *))listbox_destroy,
		(void (*)(struct control_t *))listbox_draw,
		(bool (*)(struct control_t *, bool))listbox_focus,
		(bool (*)(struct control_t *, const struct kbd_event *))listbox_handle,
		(bool (*)(struct control_t *, int, form_data_t *))listbox_get_data,
		(bool (*)(struct control_t *control, int, const void *, size_t))listbox_set_data
    };

    control_init(&listbox->control, x, y, width, height, form, &api);

	listbox->item_count = info->listbox.item_count;
	listbox->items = (char **)malloc(sizeof(char *) * listbox->item_count);
	listbox->expanded = false;

	if ((info->listbox.flags & 1) != 0)
		listbox->edit = edit_create(x, y, width - 20, height, form, info);
	else
		listbox->edit = NULL;

	for (size_t i = 0; i < listbox->item_count; i++)
		listbox->items[i] = strdup(info->listbox.items[i]);

	listbox->dropdown_height = listbox->item_count * (form_fnt->max_height + BORDER_WIDTH * 2) +
		BORDER_WIDTH * 2;
	y = listbox->control.y + listbox->control.height - BORDER_WIDTH;

	if (y + listbox->dropdown_height > DISCY) {
		if (y - listbox->control.height - listbox->dropdown_height < 0)
			y = (DISCY - listbox->dropdown_height) / 2;
		else
			y = listbox->control.y - listbox->dropdown_height + BORDER_WIDTH;
	}
	listbox->dropdown_y = y;
	listbox->selected_index = info->listbox.value;

    return listbox;
}

void listbox_destroy(listbox_t *listbox) {
	if (listbox->item_count > 0) {
		for (size_t i = 0; i < listbox->item_count; i++) {
			if (listbox->items[i])
				free(listbox->items[i]);
		}
		free(listbox->items);
	}

	if (listbox->edit)
		edit_destroy(listbox->edit);

	free(listbox);
}

static void listbox_draw_normal(listbox_t *listbox) {
	Color borderColor;
	Color bgColor;
	Color fgColor;

	if (listbox->control.focused) {
		borderColor = clRopnetDarkBrown;
		bgColor = clRopnetBrown;
		fgColor = clBlack;
	} else {
		borderColor = RGB(184, 184, 184);
		bgColor = RGB(200, 200, 200);
		fgColor = RGB(32, 32, 32);
	}
	
	int y = (listbox->control.height - form_fnt->max_height) / 2 - BORDER_WIDTH;
	
	if (listbox->edit) {
		edit_draw(listbox->edit);
		control_fill_rect(listbox->control.x + listbox->control.width - 20 - BORDER_WIDTH,
			listbox->control.y,
			20 + BORDER_WIDTH, listbox->control.height, BORDER_WIDTH,
			borderColor, bgColor);
		SetTextColor(screen, borderColor);
		TextOut(screen, listbox->control.x + listbox->control.width - 
			BORDER_WIDTH * 3 - form_fnt->max_width,
			listbox->control.y + y + 3, "\x1f");
		return;
	}
	

	control_fill_rect(listbox->control.x, listbox->control.y,
		listbox->control.width, listbox->control.height, BORDER_WIDTH,
		borderColor, bgColor);

	SetGCBounds(screen, listbox->control.x + BORDER_WIDTH, listbox->control.y + BORDER_WIDTH,
		listbox->control.width - BORDER_WIDTH * 2, listbox->control.height - BORDER_WIDTH * 2);
	//int y = (listbox->control.height - form_fnt->max_height) / 2 - BORDER_WIDTH;
	if (listbox->selected_index >= 0 && listbox->selected_index < listbox->item_count) {
		const char *text = listbox->items[listbox->selected_index];
		int w = TextWidth(form_fnt, text);
		int x = (listbox->control.width - w) / 2;
		SetTextColor(screen, fgColor);

		TextOut(screen, x, y, text);
	}
	SetTextColor(screen, borderColor);
	TextOut(screen, listbox->control.width - BORDER_WIDTH * 3 - form_fnt->max_width,
		y + 1, "\x1f");
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

static void listbox_draw_expanded(listbox_t *listbox) {
	Color borderColor = clRopnetDarkBrown;
	//Color bgColor = clRopnetBrown;

	control_fill_rect(listbox->control.x, listbox->dropdown_y,
		listbox->control.width, listbox->dropdown_height,
		BORDER_WIDTH, borderColor, -1);

	SetGCBounds(screen, listbox->control.x + BORDER_WIDTH, listbox->dropdown_y + BORDER_WIDTH,
		listbox->control.width - BORDER_WIDTH * 2, listbox->dropdown_height - BORDER_WIDTH * 2);

	int x = BORDER_WIDTH + form_fnt->max_width*2;
	int y = 0;
	SetTextColor(screen, clBlack);
	for (size_t i = 0; i < listbox->item_count; i++, y += form_fnt->max_height + BORDER_WIDTH * 2) {
		SetBrushColor(screen, i == listbox->tmp_selected_index ? clBlue : clRopnetBrown);
		SetTextColor(screen, i == listbox->tmp_selected_index ? clWhite : clBlack);
		FillBox(screen, 0, y, listbox->control.width - BORDER_WIDTH * 2,
			form_fnt->max_height + BORDER_WIDTH*2);
		TextOut(screen, x, y, listbox->items[i]);
	}
	SetGCBounds(screen, 0, 0, DISCX, DISCY);
}

void listbox_draw(listbox_t *listbox) {
	if (listbox->expanded)
		listbox_draw_expanded(listbox);
	else
		listbox_draw_normal(listbox);
}

bool listbox_focus(listbox_t *listbox, bool focus) {
	if (listbox->edit)
		edit_focus(listbox->edit, focus);
	listbox->control.focused = focus;
	listbox_draw(listbox);
	return true;
}

static void listbox_sync_edit_text(listbox_t *listbox) {
	if (listbox->edit) {
		char *text = listbox->items[listbox->selected_index];
		int textlen = strlen(text);
		edit_set_data(listbox->edit, 0, text, (size_t)textlen);
	}
}

bool listbox_handle(listbox_t *listbox, const struct kbd_event *e) {
	if (listbox->edit) {
		bool ret = edit_handle(listbox->edit, e);
		if (ret)
			return ret;
	}
	switch (e->key) {
	case KEY_ENTER:
	case KEY_SPACE:
		if (e->pressed && !e->repeated) {
			if (!listbox->expanded) {
				listbox->tmp_selected_index = listbox->selected_index;
				listbox->expanded = true;
				listbox_draw(listbox);
			} else if (e->key == KEY_ENTER) {
				listbox->selected_index = listbox->tmp_selected_index;
				listbox->expanded = false;

				if (listbox->edit)
					listbox_sync_edit_text(listbox);

				form_draw(listbox->control.form);
			}
		}
		return true;
	case KEY_ESCAPE:
		if (listbox->expanded) {
			listbox->expanded = false;
			form_draw(listbox->control.form);
			return true;
		}
	case KEY_TAB:
		return listbox->expanded;
	case KEY_DOWN:
		if (listbox->expanded && e->pressed) {
			listbox->tmp_selected_index++;
			if (listbox->tmp_selected_index >= listbox->item_count)
				listbox->tmp_selected_index = 0;
			listbox_draw(listbox);
		}
		return listbox->expanded;
	case KEY_UP:
		if (listbox->expanded && e->pressed) {
			listbox->tmp_selected_index--;
			if (listbox->tmp_selected_index < 0)
				listbox->tmp_selected_index = listbox->item_count - 1;
			listbox_draw(listbox);
		}
		return listbox->expanded;
	case KEY_PGUP:
		if (e->pressed && !listbox->expanded && listbox->item_count > 0) {
			listbox->selected_index--;
			if (listbox->selected_index < 0)
				listbox->selected_index = listbox->item_count - 1;
			if (listbox->edit)
				listbox_sync_edit_text(listbox);
			return true;
		}
		break;
	case KEY_PGDN:
		if (e->pressed && !listbox->expanded && listbox->item_count > 0) {
			listbox->selected_index++;
			if (listbox->selected_index >= listbox->item_count)
				listbox->selected_index = 0;
			if (listbox->edit)
				listbox_sync_edit_text(listbox);
			return true;
		}
		break;
	}
	return false;
}

bool listbox_get_data(listbox_t *listbox, int what, form_data_t *data) {
	if (listbox->edit)
		return edit_get_data(listbox->edit, what, data);
	switch (what) {
	case 0:
		data->data = (void *)listbox->selected_index;
		data->size = 4;
		return true;
	}
	return false;
}

bool listbox_set_data(listbox_t *listbox, int what, const void *data, size_t data_len) {
	if (listbox->edit)
		return edit_set_data(listbox->edit, what, data, data_len);
	switch (what) {
	case 0:
		listbox->selected_index = (int32_t)data;
		listbox_draw(listbox);
		return true;
	}
	return false;
}
