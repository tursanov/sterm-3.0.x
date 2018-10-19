/*
	sterm menu declarations
	(c) gsr 2000
*/

#ifndef MENU_H
#define MENU_H

#include "sterm.h"
#include "gui/scr.h"

struct menu_item{
	struct menu_item *next;
	char *text;
	int cmd;
	bool enabled;
};

struct menu{
	int top;
	int left;
	int width;
	int height;
	struct menu_item *items;
	int n_items;
	int selected;
	int max_len;
	bool centered;
};

extern struct menu_item	*new_menu_item(char *txt,int cmd,bool enabled);
extern struct menu	*new_menu(bool set_active, bool centered);
extern void		release_menu_item(struct menu_item *item);
extern void		release_menu(struct menu *m,bool unset_active);
extern struct menu	*add_menu_item(struct menu *mnu,struct menu_item *itm);
extern bool		enable_menu_item(struct menu *mnu, int cmd, bool enable);
extern bool		draw_menu(struct menu *mnu);
extern int		process_menu(struct menu *mnu,struct kbd_event *e);
extern int		get_menu_command(struct menu *mnu);
extern int		execute_menu(struct menu *mnu);

extern void		make_menu_geometry(struct menu *mnu);
extern struct menu_item	*get_menu_item(struct menu *mnu,int index);
extern bool		menu_move_up(struct menu *mnu);
extern bool		menu_move_down(struct menu *mnu);
extern bool		menu_move_home(struct menu *mnu);
extern bool		menu_move_end(struct menu *mnu);
extern bool		draw_menu_items(struct menu *mnu);

#endif		/* MENU_H */
