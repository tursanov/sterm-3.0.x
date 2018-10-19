/* Настройка параметров терминала. (c) gsr & alex 2000-2004, 2018. */

#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "gui/dialog.h"
#include "gui/exgdi.h"
#include "gui/menu.h"
#include "gui/kkt.h"
#include "gui/scr.h"

void init_kkt(void)
{
	GCPtr pGC;
//	int i;
	set_term_busy(true);
	scr_visible = false;
	set_scr_mode(m80x20, true, false);
/*	optn_cm = cmd_none;
	optn_create_menu();*/
	hide_cursor();
		pGC = CreateGC(8, 30, DISCX-16, DISCY-144);
		ClearGC(pGC, clBlack);
		DeleteGC(pGC);
/*	optn_gc = CreateGC(2, 2 * titleCY + 3, DISCX / 2 - 5, 500);
	optn_mem_gc = CreateMemGC(GetCX(optn_gc), GetCY(optn_gc));
	optn_fnt = CreateFont(_("fonts/terminal10x18.fnt"), false);
	set_optn_line_geom();
	for (i = 0; i < ASIZE(optn_groups); i++)
		optn_read_group(&cfg, i);
	optn_set_group(OPTN_GROUP_MENU);
	lprn_params_read = false;*/
}

void release_kkt(bool need_clear)
{
}

bool draw_kkt(void)
{
	return true;
}

bool process_kkt(const struct kbd_event *e)
{
	if (!e->pressed)
		return true;
	return true;
}
