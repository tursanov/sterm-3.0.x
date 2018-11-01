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
#include "kkt/kkt.h"
#include "kkt/fd/tlv.h"

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
	add_menu_item(fa_menu, new_menu_item("���������", cmd_reg_fa, true));
	add_menu_item(fa_menu, new_menu_item("���ॣ������", cmd_rereg_fa, true));
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

form_t *reg_form = NULL;
form_t *rereg_form = NULL;
form_t *open_shift_form = NULL;
form_t *close_shift_form = NULL;
form_t *cheque_corr_form = NULL;
form_t *close_fs_form = NULL;

void release_fa(void)
{
	if (reg_form)
		form_destroy(reg_form);
	if (rereg_form)
		form_destroy(rereg_form);
	if (open_shift_form)
		form_destroy(open_shift_form);
	if (close_shift_form)
		form_destroy(close_shift_form);
	if (cheque_corr_form)
		form_destroy(cheque_corr_form);
	if (close_fs_form)
		form_destroy(close_fs_form);
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
	BEGIN_FORM(open_shift_form, "����⨥ ᬥ��")
		FORM_ITEM_EDIT_TEXT(1021, "�����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "��������� �����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1023, "��� �����:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()
	form_t *form = open_shift_form;

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

		if (fd_open_shift(&p) != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("�訡��", error, dlg_yes, 0, al_center);
			form_draw(form);
		} else
			break;
	}

	fa_set_group(FAPP_GROUP_MENU);
}

void fa_close_shift() {
	BEGIN_FORM(close_shift_form, "�����⨥ ᬥ��")
		FORM_ITEM_EDIT_TEXT(1021, "�����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "��������� �����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1023, "��� �����:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()
	form_t *form = close_shift_form;

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
	fa_set_group(FAPP_GROUP_MENU);
}

void fa_calc_report() {
	form_t *form = NULL;
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
	BEGIN_FORM(close_fs_form, "�����⨥ ��")
		FORM_ITEM_EDIT_TEXT(1021, "�����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1023, "��� �����:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()
	form_t *form = close_fs_form;

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
	fa_set_group(FAPP_GROUP_MENU);
}

static const char *tax_modes[8] = { "���", "��� �����", "��� �����-������", "����", "����",
		NULL, NULL, NULL };
static const char *short_modes[8] = { "���", "�����.�.", "�������.�.",
		"������", "���", "��������", NULL, NULL };
static const char *modes[8] = { "���஢����", "��⮭���� ०��", "��⮬���᪨� ०��",
		"�ਬ������ � ��� ���", "����� ���", "�ਬ������ � ���୥�", NULL, NULL };

void fa_registration() {
	BEGIN_FORM(reg_form, "���������")
		FORM_ITEM_EDIT_TEXT(1048, "������������ ���짮��⥫�:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1018, "��� ���짮��⥫�:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1009, "���� ���⮢:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1187, "���� ���⮢:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_BITSET(1062, "���⥬� ���������������:", tax_modes, tax_modes, 5, 0)
		FORM_ITEM_EDIT_TEXT(1037, "�������樮��� ����� ���:", NULL, FORM_INPUT_TYPE_NUMBER, 16)
		FORM_ITEM_BITSET(9999, "������ ࠡ���:", short_modes, modes, 6, 0)
		FORM_ITEM_EDIT_TEXT(1036, "����� ��⮬��:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1021, "�����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1023, "��� �����:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1060, "���� ᠩ� ���:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1117, "���� �. ����� ���. 祪�:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1046, "������������ ���:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1017, "��� ���:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()
	form_t *form = reg_form;

	while (form_execute(form) == 1) {
		form_text_t cashier;
		form_text_t inn;
		fd_registration_params_t p;

		form_get_text(form, 1021, &cashier, true);
		form_get_text(form, 1023, &inn, false);

		memcpy(p.cashier, cashier.text, cashier.length);
		p.cashier[cashier.length] = 0;

		if (inn.length > 0)
			memcpy(p.cashier_inn, inn.text, inn.length);
		p.cashier_inn[inn.length] = 0;

		printf("�����: %s\n", cashier.text);
		printf("��� �����: %s\n", inn.text);

	}
	fa_set_group(FAPP_GROUP_MENU);
}

static const char *short_rereg_reason[8] = { "������ ��", "������ ���", "���.����.",
	"���.�����.���", NULL,  NULL, NULL, NULL };
static const char *rereg_reason[8] = { "������ ��", "������ ���", "��������� ४����⮢",
	"��������� ����஥� ���", NULL,  NULL, NULL, NULL };

static size_t get_trim_string_size(const ffd_tlv_t *tlv) {
	size_t size = tlv->length;
	const char *s = FFD_TLV_DATA_AS_STRING(tlv) + size - 1;
	while (size != 0 && *s-- == ' ')
		size--;
	return size;
}

void fa_reregistration() {
	bool empty = rereg_form == NULL;
	BEGIN_FORM(rereg_form, "���ॣ������")
		FORM_ITEM_EDIT_TEXT(1048, "������������ ���짮��⥫�:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1018, "��� ���짮��⥫�:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1009, "���� ���⮢:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1187, "���� ���⮢:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_BITSET(1062, "���⥬� ���������������:", tax_modes, tax_modes, 5, 0)
		FORM_ITEM_EDIT_TEXT(1037, "�������樮��� ����� ���:", NULL, FORM_INPUT_TYPE_NUMBER, 16)
		FORM_ITEM_BITSET(9999, "������ ࠡ���:", short_modes, modes, 6, 0)
		FORM_ITEM_EDIT_TEXT(1036, "����� ��⮬��:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1021, "�����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1023, "��� �����:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1060, "���� ᠩ� ���:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1117, "���� �. ����� ���. 祪�:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1046, "������������ ���:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1017, "��� ���:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BITSET(9998, "��稭� ���ॣ����樨", short_rereg_reason, rereg_reason, 4, 0)
		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()
	form_t *form = rereg_form;

	if (empty) {
		uint8_t data[2048];
		size_t data_len = sizeof(data);
		uint8_t modes = 0;

		int ret;
		if ((ret = kkt_get_last_reg_data(data, &data_len)) == 0 && data_len > 0) {
			for (const ffd_tlv_t *tlv = (ffd_tlv_t *)data, *end = (ffd_tlv_t *)(data + data_len);
				tlv < end; tlv = FFD_TLV_NEXT(tlv)) {
				switch (tlv->tag) {
				case 1048:
					FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv), tlv->length);
					break;
				case 1018:
					FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv),
						get_trim_string_size(tlv));
					break;
				case 1009:
					FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv), tlv->length);
					break;
				case 1187:
					FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv), tlv->length);
					break;
				case 1062:
					FORM_BITSET_SET_VALUE(form, tlv->tag, FFD_TLV_DATA_AS_UINT8(tlv));
					break;
				case 1037:
					FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv),
						get_trim_string_size(tlv));
					break;
				case 1056:
					if (FFD_TLV_DATA_AS_UINT8(tlv))
						modes |= 0x01;
					break;
				case 1002:
					if (FFD_TLV_DATA_AS_UINT8(tlv))
						modes |= 0x02;
					break;
				case 1001:
					if (FFD_TLV_DATA_AS_UINT8(tlv))
						modes |= 0x04;
					break;
				case 1109:
					if (FFD_TLV_DATA_AS_UINT8(tlv))
						modes |= 0x08;
					break;
				case 1110:
					if (FFD_TLV_DATA_AS_UINT8(tlv))
						modes |= 0x10;
					break;
				case 1108:
					if (FFD_TLV_DATA_AS_UINT8(tlv))
						modes |= 0x20;
					break;
				case 1036:
					FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv), tlv->length);
					break;
				case 1060:
					FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv), tlv->length);
					break;
				case 1117:
					FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv), tlv->length);
					break;
				case 1046:
					FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv), tlv->length);
					break;
				case 1017:
					FORM_EDIT_TEXT_SET_TEXT(form, tlv->tag, FFD_TLV_DATA_AS_STRING(tlv),
						get_trim_string_size(tlv));
					break;
				}
			}
			FORM_BITSET_SET_VALUE(form, 9999, modes);
		}
	}

	while (form_execute(form) == 1) {
		form_text_t cashier;
		form_text_t inn;
		fd_registration_params_t p;

		form_get_text(form, 1021, &cashier, true);
		form_get_text(form, 1023, &inn, false);

		memcpy(p.cashier, cashier.text, cashier.length);
		p.cashier[cashier.length] = 0;

		if (inn.length > 0)
			memcpy(p.cashier_inn, inn.text, inn.length);
		p.cashier_inn[inn.length] = 0;

		printf("�����: %s\n", cashier.text);
		printf("��� �����: %s\n", inn.text);

	}
	fa_set_group(FAPP_GROUP_MENU);
}

void fa_cheque_corr() {
	const char *pay_type[] = { "���४�� ��室�", "���४�� ��室�" };
	const char *tax_mode[] = { "���", "��� �����", "��� �����-������", "����", "����" };
	const char *corr_type[] = { "�������⥫쭮", "�� �।��ᠭ��" };

	BEGIN_FORM(cheque_corr_form, "��� ���४樨")
		FORM_ITEM_LISTBOX(1054, "�ਧ��� ����:", pay_type, ASIZE(pay_type), -1)
		FORM_ITEM_LISTBOX(1055, "���⥬� ���������������:", tax_mode, ASIZE(tax_mode), -1)
		FORM_ITEM_LISTBOX(1173, "��� ���४樨:", corr_type, ASIZE(corr_type), -1)
		FORM_ITEM_EDIT_TEXT(1177, "���ᠭ�� ���४樨:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1178, "��� ���४樨:", NULL, FORM_INPUT_TYPE_DATE, 10)
		FORM_ITEM_EDIT_TEXT(1179, "����� �।��ᠭ��:", NULL, FORM_INPUT_TYPE_TEXT, 32)

		FORM_ITEM_EDIT_TEXT(1031, "�����묨:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1081, "��������묨:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1215, "�।����⮩:", NULL, FORM_INPUT_TYPE_NUMBER, 16)
		FORM_ITEM_EDIT_TEXT(1216, "���⮯��⮩:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1217, "������ �।�⠢������:", NULL, FORM_INPUT_TYPE_TEXT, 256)

		FORM_ITEM_EDIT_TEXT(1102, "��� 20%:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1103, "��� 10%:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1104, "��� 0%:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1105, "��� ���:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1106, "��� 20/120:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1107, "��� 10/110:", NULL, FORM_INPUT_TYPE_TEXT, 256)

		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()
	form_t *form = cheque_corr_form;

	while (form_execute(form) == 1) {
		form_text_t cashier;
		form_text_t inn;
		fd_registration_params_t p;

		form_get_text(form, 1021, &cashier, true);
		form_get_text(form, 1023, &inn, false);

		memcpy(p.cashier, cashier.text, cashier.length);
		p.cashier[cashier.length] = 0;

		if (inn.length > 0)
			memcpy(p.cashier_inn, inn.text, inn.length);
		p.cashier_inn[inn.length] = 0;

		printf("�����: %s\n", cashier.text);
		printf("��� �����: %s\n", inn.text);

	}
	fa_set_group(FAPP_GROUP_MENU);
}


bool process_fa(const struct kbd_event *e)
{
	if (!e->pressed)
		return true;
	if (fa_active_group == FAPP_GROUP_MENU) {
		if (process_menu(fa_menu, (struct kbd_event *)e) != cmd_none)
			switch (get_menu_command(fa_menu)){
				case cmd_reg_fa:
					fa_registration();
					break;
				case cmd_rereg_fa:
					fa_reregistration();
					break;
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
				case cmd_cheque_corr_fa:
					fa_cheque_corr();
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
