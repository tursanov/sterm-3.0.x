/* ����ன�� ��ࠬ��஢ �ନ����. (c) gsr & alex 2000-2004, 2018. */

#include <sys/socket.h>
#include <sys/param.h>
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
#include <limits.h>
#include "sysdefs.h"
#include "gui/dialog.h"
#include "gui/exgdi.h"
#include "gui/fa.h"
#include "gui/menu.h"
#include "gui/scr.h"
#include "gui/forms.h"
#include "kkt/fd/fd.h"

static int fa_active_group = -1;
static int fa_active_item = -1;
//static int fa_active_control = -1;
int fa_cm = cmd_none;
static struct menu *fa_menu = NULL;

/* ��㯯� ��ࠬ��஢ ����ன�� */
enum {
	FAPP_GROUP_MENU = -1,
};

/* ����� � ���� ����஥� */
static bool fa_create_menu(void)
{
	fa_menu = new_menu(false, false);
	add_menu_item(fa_menu, new_menu_item("���������/���ॣ������", cmd_reg_fa, true));
	add_menu_item(fa_menu, new_menu_item("����⨥ ᬥ��", cmd_open_shift_fa, true));
	add_menu_item(fa_menu, new_menu_item("�����⨥ ᬥ��", cmd_close_shift_fa, true));
	add_menu_item(fa_menu, new_menu_item("���", cmd_cheque_fa, true));
	add_menu_item(fa_menu, new_menu_item("��� ���४樨", cmd_cheque_corr_fa, true));
	add_menu_item(fa_menu, new_menu_item("���� � ⥪��e� ���ﭨ� ���⮢", cmd_calc_report_fa, true));
	add_menu_item(fa_menu, new_menu_item("�����⨥ ��", cmd_close_fs_fa, true));
	add_menu_item(fa_menu, new_menu_item("��室", cmd_exit, true));
	return true;
}

static bool fa_set_group(int n)
{
	bool ret = false;
	fa_cm = cmd_none;
	fa_active_group = n;
	fa_active_item = 0;
	if (n == FAPP_GROUP_MENU){
		ClearScreen(clBlack);
		draw_menu(fa_menu);
		ret = true;
	}
	return ret;
}


bool init_fa(void)
{
	fa_active = true;
	hide_cursor();
	scr_visible = false;
	scr_visible = false;
	set_scr_mode(m80x20, true, false);
	set_term_busy(true);
	ClearScreen(clBlack);

	fa_create_menu();
	fa_set_group(FAPP_GROUP_MENU);

	return true;
}

void release_fa(void)
{
	release_menu(fa_menu,false);
	online = true;
	pop_term_info();
	ClearScreen(clBtnFace);
	fa_active = false;
}

bool draw_fa(void)
{
	if (fa_active_group == FAPP_GROUP_MENU)
		return draw_menu(fa_menu);
    return true;
}

void fa_open_shift() {
	BEGIN_FORM(form, "����⨥ ᬥ��")
		FORM_ITEM_EDIT_TEXT(1021, "�����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "��������� �����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1023, "��� �����:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()

	while (form_execute(form) == 1) {
		form_text_t cashier;
		form_text_t post;
		form_text_t inn;
		fd_shift_params_t p;

		form_get_text(form, 1021, &cashier, true);
		form_get_text(form, 9999, &post, true);
		form_get_text(form, 1023, &inn, false);

		memcpy(p.cashier, cashier.text, cashier.length);
		if (post.length > 0 && cashier.length < 63) {
			size_t l = 64 - cashier.length - 1;
			l = MIN(l, post.length);

			p.cashier[cashier.length] = ' ';
			memcpy(p.cashier + cashier.length + 1, post.text, l);
			p.cashier[cashier.length + l + 1] = 0;
		} else
			p.cashier[cashier.length] = 0;

		if (inn.length > 0)
			memcpy(p.cashier_inn, inn.text, inn.length);
		p.cashier_inn[inn.length] = 0;

		printf("�����: %s\n", cashier.text);
		printf("���������: %s\n", post.text);
		printf("��� �����: %s\n", inn.text);

		if (fd_open_shift(&p) != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("�訡��", error, dlg_yes, 0, al_center);
			form_draw(form);
		} else
			break;
	}
	form_destroy(form);
	fa_set_group(FAPP_GROUP_MENU);
}

void fa_close_shift() {
	BEGIN_FORM(form, "�����⨥ ᬥ��")
		FORM_ITEM_EDIT_TEXT(1021, "�����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "��������� �����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1023, "��� �����:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()

	while (form_execute(form) == 1) {
		form_text_t cashier;
		form_text_t post;
		form_text_t inn;
		fd_shift_params_t p;

		form_get_text(form, 1021, &cashier, true);
		form_get_text(form, 9999, &post, true);
		form_get_text(form, 1023, &inn, false);

		memcpy(p.cashier, cashier.text, cashier.length);
		if (post.length > 0 && cashier.length < 63) {
			size_t l = 64 - cashier.length - 1;
			l = MIN(l, post.length);

			p.cashier[cashier.length] = ' ';
			memcpy(p.cashier + cashier.length + 1, post.text, l);
			p.cashier[cashier.length + l + 1] = 0;
		} else
			p.cashier[cashier.length] = 0;

		if (inn.length > 0)
			memcpy(p.cashier_inn, inn.text, inn.length);
		p.cashier_inn[inn.length] = 0;

		printf("�����: %s\n", cashier.text);
		printf("���������: %s\n", post.text);
		printf("��� �����: %s\n", inn.text);

		if (fd_close_shift(&p) != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("�訡��", error, dlg_yes, 0, al_center);
			form_draw(form);
		} else
			break;
	}
	form_destroy(form);
	fa_set_group(FAPP_GROUP_MENU);
}

void fa_calc_report() {
	BEGIN_FORM(form, "���� � ⥪�饬 ���ﭨ� ���⮢")
		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()

	while (form_execute(form) == 1) {
		if (fd_calc_report() != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("�訡��", error, dlg_yes, 0, al_center);
			form_draw(form);
		} else
			break;
	}
	form_destroy(form);
	fa_set_group(FAPP_GROUP_MENU);
}

void fa_close_fs() {
	BEGIN_FORM(form, "�����⨥ ��")
		FORM_ITEM_EDIT_TEXT(1021, "�����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1023, "��� �����:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()

	while (form_execute(form) == 1) {
		form_text_t cashier;
		form_text_t inn;
		fd_close_fs_params_t p;

		form_get_text(form, 1021, &cashier, true);
		form_get_text(form, 1023, &inn, false);

		memcpy(p.cashier, cashier.text, cashier.length);
		p.cashier[cashier.length] = 0;

		if (inn.length > 0)
			memcpy(p.cashier_inn, inn.text, inn.length);
		p.cashier_inn[inn.length] = 0;

		printf("�����: %s\n", cashier.text);
		printf("��� �����: %s\n", inn.text);

		if (fd_close_fs(&p) != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("�訡��", error, dlg_yes, 0, al_center);
			form_draw(form);
		} else
			break;
	}
	form_destroy(form);
	fa_set_group(FAPP_GROUP_MENU);
}


bool process_fa(const struct kbd_event *e)
{
	if (!e->pressed)
		return true;
	if (fa_active_group == FAPP_GROUP_MENU) {
		if (process_menu(fa_menu, (struct kbd_event *)e) != cmd_none)
			switch (get_menu_command(fa_menu)){
				case cmd_open_shift_fa:
					fa_open_shift();
					break;
				case cmd_close_shift_fa:
					fa_close_shift();
					break;
				case cmd_calc_report_fa:
					fa_calc_report();
					break;
				case cmd_close_fs_fa:
					fa_close_fs();
					break;
				/*case cmd_sys_fa:
					fa_set_group(OPTN_GROUP_SYSTEM);
					break;
				case cmd_dev_fa:
					get_lprn_params();
					fa_set_group(OPTN_GROUP_DEVICES);
					break;
				case cmd_tcpip_fa:
					fa_set_group(OPTN_GROUP_TCPIP);
					break;
				case cmd_ppp_fa:
					fa_set_group(OPTN_GROUP_PPP);
					break;
				case cmd_bank_fa:
					fa_set_group(OPTN_GROUP_BANK);
					break;
				case cmd_kkt_fa:
					fa_set_group(OPTN_GROUP_KKT);
					break;
				case cmd_scr_fa:
					fa_set_group(OPTN_GROUP_SCR);
					break;
				case cmd_kbd_fa:
					fa_set_group(OPTN_GROUP_KBD);
					break;
				case cmd_store_fa:
					fa_cm = cmd_store_fa;*/	/* fall through */
				default:
					return false;
			}
		else
			return true;
    }
	return true;
}
