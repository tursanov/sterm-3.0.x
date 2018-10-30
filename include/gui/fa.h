/* KKT (c) gsr & alex 2000-2004, 2018 */

#if !defined GUI_FA_H
#define GUI_FA_H

#include "cfg.h"
#include "kbd.h"

extern bool	init_fa(void);
extern void	release_fa(void);
extern bool	draw_fa(void);
extern bool	process_fa(const struct kbd_event *e);

#endif		/* GUI_KKT_H */
