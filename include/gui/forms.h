#if !defined GUI_FORMS_H
#define GUI_FORMS_H

typedef struct form_t form_t;

struct kbd_event;

typedef enum form_item_type_t {
	FORM_ITEM_TYPE_NONE,
	FORM_ITEM_TYPE_EDIT_TEXT,
	FORM_ITEM_TYPE_BUTTON,
} form_item_type_t;

typedef enum form_input_type_t {
	FORM_INPUT_TYPE_TEXT = 0,
	FORM_INPUT_TYPE_NUMBER
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
} form_item_info_t;

#define BEGIN_FORM(x, name) form_t *x = NULL; { \
	form_t **__f = &x; const char *__n = name; form_item_info_t __items[] = {
#define FORM_ITEM_EDIT_TEXT(id, name, text, input_type, max_length) { FORM_ITEM_TYPE_EDIT_TEXT, \
	id, name, text, input_type, max_length, NULL },
#define FORM_ITEM_BUTTON(id, text, action) { FORM_ITEM_TYPE_BUTTON, id, NULL, text, 0, 0, action },
#define END_FORM() }; *__f = form_create(__n, __items, ASIZE(__items)); }

form_t* form_create(const char *name, form_item_info_t items[], size_t item_count);
void form_destroy(form_t *form);
int form_execute(form_t *form);
void form_draw(form_t *form);

typedef struct form_text_t {
	const char *text;
	size_t length;
} form_text_t;

bool form_get_text(form_t *form, int id, form_text_t *text, bool trim);

#endif		/* GUI_FORMS_H */
