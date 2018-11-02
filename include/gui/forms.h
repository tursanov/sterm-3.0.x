#if !defined GUI_FORMS_H
#define GUI_FORMS_H

typedef struct form_t form_t;

struct kbd_event;

typedef enum form_item_type_t {
	FORM_ITEM_TYPE_NONE,
	FORM_ITEM_TYPE_EDIT_TEXT,
	FORM_ITEM_TYPE_BUTTON,
	FORM_ITEM_TYPE_LISTBOX,
	FORM_ITEM_TYPE_BITSET,
} form_item_type_t;

typedef enum form_input_type_t {
	FORM_INPUT_TYPE_TEXT = 0,
	FORM_INPUT_TYPE_NUMBER,
	FORM_INPUT_TYPE_DATE,
	FORM_INPUT_TYPE_MONEY,
} form_input_type_t;

typedef bool (*form_action_t)(form_t *form);

typedef struct form_item_info_t {
	form_item_type_t type;
	int id;
	const char *name;
	const char *text;
	form_input_type_t input_type;
	size_t max_length;
	form_action_t action;
	const char **short_items;
	const char **items;
	int value;
} form_item_info_t;

#define BEGIN_FORM(x, name) { \
	form_t **__f = &x; if (x == NULL) { \
		const char *__n = name; form_item_info_t __items[] = {
#define FORM_ITEM_EDIT_TEXT(id, name, text, input_type, max_length) { FORM_ITEM_TYPE_EDIT_TEXT, \
	id, name, text, input_type, max_length, NULL, NULL, NULL, 0 },
#define FORM_ITEM_BUTTON(id, text, action) { FORM_ITEM_TYPE_BUTTON, \
	id, NULL, text, 0, 0, action, NULL, NULL, 0 },
#define FORM_ITEM_LISTBOX(id, name, items, item_count, value) \
	{ FORM_ITEM_TYPE_LISTBOX, \
	id, name, NULL, 0, item_count, NULL, NULL, items, value },
#define FORM_ITEM_BITSET(id, name, short_items, items, item_count, value) \
	{ FORM_ITEM_TYPE_BITSET, \
	id, name, NULL, 0, item_count, NULL, short_items, items, value },
#define END_FORM() }; *__f = form_create(__n, __items, ASIZE(__items)); } \
	else form_draw(*__f); }


form_t* form_create(const char *name, form_item_info_t items[], size_t item_count);
void form_destroy(form_t *form);
int form_execute(form_t *form);
void form_draw(form_t *form);
bool form_focus(form_t *form, int id);

typedef struct form_data_t {
	const void *data;
	size_t size;
} form_data_t;

bool form_get_data(form_t *form, int id, int what, form_data_t *data);

static inline int form_get_int_data(form_t *form, int id, int what, int default_value) {
	form_data_t data;
	if (form_get_data(form, id, what, &data))
		return (int)data.data;
	return default_value;
}
bool form_set_data(form_t *form, int id, int what, const void *data, size_t data_size);

#define FORM_EDIT_TEXT_SET_TEXT(form, id, text, text_size) \
	form_set_data(form, id, 0, text, text_size)
#define FORM_BITSET_SET_VALUE(form, id, value) \
	form_set_data(form, id, 0, (void *)(int)value, 0)
#define FORM_LISTBOX_SET_SELECTED_INDEX(form, id, selected_index) \
	form_set_data(form, id, 0, (void *)(int)selected_index, 0)

#endif		/* GUI_FORMS_H */
