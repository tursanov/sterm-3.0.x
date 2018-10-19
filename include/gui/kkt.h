/* KKT (c) gsr & alex 2000-2004, 2018 */

#if !defined GUI_KKT_H
#define GUI_KKT_H

#include "cfg.h"
#include "kbd.h"

extern void	init_kkt(void);
extern void	release_kkt(bool need_clear);
extern bool	draw_kkt(void);
extern bool	process_kkt(const struct kbd_event *e);

#endif		/* GUI_OPTIONS_H */
