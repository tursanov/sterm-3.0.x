#ifndef WINDOW_H
#define WINDOW_H

#include "controls.h"

struct window_t;
typedef struct window_t window_t;
typedef bool (*window_handle_event_func_t)(window_t *w, const struct kbd_event *e);

window_t *window_create(GCPtr gc, const char *title, window_handle_event_func_t handle_event_func);
void window_destroy(window_t *w);
void window_set_dialog_result(window_t *w, int result);
void window_add_control(window_t *w, control_t *c);
void window_add_label(window_t *w, int x, int y, const char *text);
void window_add_label_with_id(window_t *w, int id, int x, int y, const char *text);
void window_draw(window_t *w);


int window_show_dialog(window_t *w);
GCPtr window_get_gc(window_t *w);
control_t *window_get_control(window_t *w, int id);
void window_set_label_text(window_t *w, int id, const char *text);

bool window_set_focus(window_t *w, int id);

bool window_get_data(window_t *w, int id, int what, data_t *data);
bool window_set_data(window_t *w, int id, int what, const void *data, size_t data_size);

static inline int window_get_int_data(window_t *w, int id, int what, int default_value) {
	data_t data;
	if (window_get_data(w, id, what, &data))
		return (int)data.data;
	return default_value;
}

static inline void * window_get_ptr_data(window_t *w, int id, int what) {
	data_t data;
	if (window_get_data(w, id, what, &data))
		return (void *)data.data;
	return NULL;
}



#endif
