#ifndef CONTROL_H
#define CONTROL_H

struct control_t;
typedef struct control_api_t {
	void (*destroy)(struct control_t *control);
	void (*draw)(struct control_t *control);
	bool (*focus)(struct control_t *control, bool focus);
	bool (*handle)(struct control_t *control, const struct kbd_event *e);
	bool (*get_text)(struct control_t *control, form_text_t *text, bool trim);
} control_api_t;

typedef struct control_t {
	int x;
   	int y;
   	int width;
   	int height;
   	control_api_t api;
   	bool focused;
   	form_t *form;
} control_t;

void control_init(control_t *control, int x, int y, int width, int height, 
	form_t *form, control_api_t* api);

control_t *control_create(int x, int y, int w, int h, form_t *form, form_item_info_t *info);
void control_destroy(control_t *control);
void control_draw(control_t *control);
bool control_focus(control_t *control, bool focus);
bool control_handle(struct control_t *control, const struct kbd_event *e);
bool control_get_text(struct control_t *control, form_text_t *text, bool trim);


#endif // CONTROL_H