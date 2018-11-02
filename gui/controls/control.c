#include "control.h"

void control_init(control_t *control,
		int x, int y, int width, int height,
		form_t *form,
		control_api_t* api) {
	control->x = x;
	control->y = y;
	control->width = width;
	control->height = height;
	control->api = *api;
	control->focused = false;
	control->form = form;
}

typedef control_t *(*create_control_func_t)(int x, int y, int w, int h, form_t *form,
	form_item_info_t *);

static struct {
	form_item_type_t type;
	create_control_func_t create_control;
} control_create_func[] = {
	{ FORM_ITEM_TYPE_EDIT_TEXT, (create_control_func_t)edit_create },
	{ FORM_ITEM_TYPE_BUTTON, (create_control_func_t)button_create },
	{ FORM_ITEM_TYPE_LISTBOX, (create_control_func_t)listbox_create },
	{ FORM_ITEM_TYPE_BITSET, (create_control_func_t)bitset_create },
};

control_t *control_create(int x, int y, int w, int h, form_t *form, form_item_info_t *info) {
	for (int i = 0; i < ASIZE(control_create_func); i++) {
		if (control_create_func[i].type == info->type) {
			control_t *control = control_create_func[i].create_control(x, y, w, h, form, info);
			return control;
		}
	}
	return NULL;
}

void control_destroy(control_t *control) {
	control->api.destroy(control);
}

void control_draw(control_t *control) {
	control->api.draw(control);
}

bool control_focus(control_t *control, bool focus) {
	return control->api.focus(control, focus);
}

bool control_handle(struct control_t *control, const struct kbd_event *e) {
	return control->api.handle(control, e);
}

bool control_get_data(struct control_t *control, int what, form_data_t *data) {
	if (control->api.get_data)
		return control->api.get_data(control, what, data);
	return false;
}

bool control_set_data(struct control_t *control, int what, const void *data, size_t data_len) {
	if (control->api.set_data)
		return control->api.set_data(control, what, data, data_len);
	return false;
}

void control_fill_rect(int x, int y, int width, int height, int border_width,
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