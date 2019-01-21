#ifndef CONTROL_H
#define CONTROL_H

#include "gui/gdi.h"

#define BORDER_WIDTH	2

typedef struct data_t {
	void *data;
	size_t size;
} data_t;

struct control_t;
typedef struct control_api_t {
	void (*destroy)(struct control_t *control);
	void (*draw)(struct control_t *control);
	bool (*focus)(struct control_t *control, bool focus);
	bool (*handle)(struct control_t *control, const struct kbd_event *e);
	bool (*get_data)(struct control_t *control, int what, data_t *data);
	bool (*set_data)(struct control_t *control, int what, const void *data, size_t data_len);
} control_api_t;

typedef struct control_t {
	int x;
   	int y;
   	int width;
   	int height;
	GCPtr gc;
   	control_api_t api;
   	bool focused;
	void *extra;
	void (*refresh_parent_fn)(struct control_t *);
} control_t;

void control_init(control_t *control, GCPtr gc, int x, int y, int width, int height,
	control_api_t* api);

#define CONTROL_EXTRA(c, type) ((type *)control_get_extra(c))

void control_destroy(control_t *control);
void control_draw(control_t *control);
bool control_focus(control_t *control, bool focus);
bool control_handle(struct control_t *control, const struct kbd_event *e);
void* control_get_extra(struct control_t *control);
void control_set_extra(struct control_t *control, void *extra);
bool control_get_data(struct control_t *control, int what, data_t *data);
bool control_set_data(struct control_t *control, int what, const void *data, size_t data_len);
void control_set_refresh_parent(struct control_t *control, void (*fn)(struct control_t *));
void control_refresh_parent(struct control_t *control);

void fill_rect(GCPtr screen, int x, int y, int width, int height, int border_width,
		Color border_color, int bg_color);

#endif // CONTROL_H
