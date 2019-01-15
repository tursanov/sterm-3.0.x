/* KKT (c) gsr & alex 2000-2004, 2018 */

#if !defined GUI_FA_H
#define GUI_FA_H

#include "cfg.h"
#include "kbd.h"

extern bool	init_fa(int arg);
extern void	release_fa(void);
extern bool	draw_fa(void);
extern bool	process_fa(const struct kbd_event *e);

extern bool cashier_load();
extern bool cashier_save();
extern bool cashier_set(const char *name, const char *post, const char *inn);
extern bool cashier_set_name(const char *name);
extern const char *cashier_get_name();
extern const char *cashier_get_post();
extern const char *cashier_get_inn();
extern const char *cashier_get_cashier();

#endif		/* GUI_KKT_H */
