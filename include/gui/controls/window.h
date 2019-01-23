#ifndef WINDOW_H
#define WINDOW_H

#include "controls.h"

struct window_t;
typedef struct window_t window_t;

window_t *window_create(GCPtr gc, const char *title);
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

#endif
