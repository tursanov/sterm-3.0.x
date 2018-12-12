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
#include "gui/cheque.h"
#include "kkt/fd/fd.h"
#include "kkt/kkt.h"
#include "kkt/fdo.h"
#include "kkt/fd/tlv.h"

static int fa_active_group = -1;
static int fa_active_item = -1;
//static int fa_active_control = -1;
int fa_cm = cmd_none;
static struct menu *fa_menu = NULL;
int fa_arg = cmd_fa;

static bool process_fa_cmd(int cmd);

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
	if (fa_arg != cmd_fa)
		return false;
	bool ret = false;
	fa_cm = cmd_none;
	fa_active_group = n;
	if (n == FAPP_GROUP_MENU){
		ClearScreen(clBlack);
		draw_menu(fa_menu);
		ret = true;
	}
	return ret;
}


static uint8_t rereg_data[2048];
static size_t rereg_data_len = sizeof(rereg_data);
static int64_t user_inn = 0;

static int fa_get_reregistration_data() {
	int ret;
	rereg_data_len = sizeof(rereg_data);
	if ((ret = kkt_get_last_reg_data(rereg_data, &rereg_data_len)) == 0 && rereg_data_len > 0) {
		for (const ffd_tlv_t *tlv = (ffd_tlv_t *)rereg_data,
				*end = (ffd_tlv_t *)(rereg_data + rereg_data_len);
				tlv < end;
				tlv = FFD_TLV_NEXT(tlv)) {
			switch (tlv->tag) {
				case 1018:
					user_inn = atoll(FFD_TLV_DATA_AS_STRING(tlv));
					break;
			}
		}
		return 0;
	}

	return ret;
}

bool init_fa(int arg)
{
	fa_arg = arg;
	fa_active = true;
	hide_cursor();
	scr_visible = false;
	scr_visible = false;
	set_scr_mode(m80x20, true, false);
	set_term_busy(true);

	fa_get_reregistration_data();

	if (arg == cmd_fa) {
		ClearScreen(clBlack);
		fa_create_menu();
		if (_ad->clist.count > 0) {
			fa_active_item = 4;
			fa_menu->selected = fa_active_item;
			printf("fa_active_item = %d\n", fa_active_item);
		}
		fa_set_group(FAPP_GROUP_MENU);
	} else {
		fa_menu = NULL;
		process_fa_cmd(arg);
	}

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
	if (reg_form) {
		form_destroy(reg_form);
		reg_form = NULL;
	}
	if (rereg_form) {
		form_destroy(rereg_form);
		rereg_form = NULL;
	}
	if (open_shift_form) {
		form_destroy(open_shift_form);
		open_shift_form = NULL;
	}
	if (close_shift_form) {
		form_destroy(close_shift_form);
		close_shift_form = NULL;
	}
	if (cheque_corr_form) {
		form_destroy(cheque_corr_form);
		cheque_corr_form = NULL;
	}
	if (close_fs_form) {
		form_destroy(close_fs_form);
		close_fs_form = NULL;
	}
	cheque_release();
	if (fa_menu)
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

static int fa_get_string(form_t *form, form_data_t *data, uint16_t tag, bool required) {
	if (!form_get_data(form, tag, 1, data)) {
		form_focus(form, tag);
		message_box("�訡��", "������ ������ ��", dlg_yes, 0, al_center);
		form_draw(form);
		return -1;
	}

	if (data->size == 0) {
		if (required) {
			form_focus(form, tag);
			message_box("�訡��", "��易⥫쭮� ���� �� ���������", dlg_yes, 0, al_center);
			form_draw(form);
			return -2;
		}
	}
	return 0;
}

static int fa_tlv_add_string(form_t *form, uint16_t tag, bool required) {
	int ret;
	form_data_t data;

	if ((ret = fa_get_string(form, &data, tag, required)) != 0)
		return ret;

	if (data.size == 0)
		return 0;

	if ((ret = ffd_tlv_add_string(tag, (const char *)data.data)) != 0) {
		form_focus(form, tag);
		message_box("�訡��", "�訡�� �� ���������� TLV. ������� � ࠧࠡ��稪��", dlg_yes, 0, al_center);
		form_draw(form);
		return ret;
	}
	return 0;
}

static int fa_tlv_add_fixed_string(form_t *form, uint16_t tag, size_t fixed_length, bool required) {
	int ret;
	form_data_t data;

	if ((ret = fa_get_string(form, &data, tag, required)) != 0)
		return ret;
		
	if (data.size == 0)
		return 0;
		
	if ((ret = ffd_tlv_add_fixed_string(tag, (const char *)data.data, fixed_length)) != 0) {
		form_focus(form, tag);
		message_box("�訡��", "�訡�� �� ���������� TLV. ������� � ࠧࠡ��稪��", dlg_yes, 0, al_center);
		form_draw(form);
		return ret;
	}
	return 0;
}

static uint64_t form_data_to_vln(form_data_t *data) {
	double v = atof(data->data);
	uint64_t value = (uint64_t)((v + 0.009) * 100.0);
/*	
	uint64_t d = 1;
	
	const char *text = (const char *)data->data + data->size - 1;
	for (int i = 0; i < data->size; i++, text--, d = d * 10) {
		if (isdigit(*text))
			value += (*text - '0') * d;
		else if (*text == '.' || *text == ',') {
		}
	}*/
	return value;
}

static int fa_tlv_add_vln(form_t *form, uint16_t tag, bool required) {
	form_data_t data;

	if (!form_get_data(form, tag, 1, &data)) {
		form_focus(form, tag);
		message_box("�訡��", "������ ������ ��", dlg_yes, 0, al_center);
		form_draw(form);
		return -1;
	}

	if (data.size == 0) {
		if (required) {
			form_focus(form, tag);
			message_box("�訡��", "��易⥫쭮� ���� �� ���������", dlg_yes, 0, al_center);
			form_draw(form);
			return -2;
		}
		return 0;
	}

	uint64_t value = form_data_to_vln(&data);

	int ret;
	if ((ret = ffd_tlv_add_vln(tag, value)) != 0) {
		form_focus(form, tag);
		message_box("�訡��", "���ࠢ��쭮� ���祭��", dlg_yes, 0, al_center);
		form_draw(form);
		return ret;
	}
	return 0;
}

static int fa_tlv_add_unixtime(form_t *form, uint16_t tag, bool required) {
	form_data_t data;

	if (!form_get_data(form, tag, 1, &data)) {
		form_focus(form, tag);
		message_box("�訡��", "������ ������ ��", dlg_yes, 0, al_center);
		form_draw(form);
		return -1;
	}

	if (data.size == 0) {
		if (required) {
			form_focus(form, tag);
			message_box("�訡��", "��易⥫쭮� ���� �� ���������", dlg_yes, 0, al_center);
			form_draw(form);
			return -2;
		}
		return 0;
	}

	struct tm tm;
	char *s;
	if ((s = strptime((const char *)data.data, "%d.%m.%Y", &tm)) == NULL || *s != 0) {
		form_focus(form, tag);
		message_box("�訡��", "���ࠢ��쭮� ���祭��", dlg_yes, 0, al_center);
		form_draw(form);
		return -1;
	}

	time_t value = timelocal(&tm);

	int ret;
	if ((ret = ffd_tlv_add_unix_time(tag, value)) != 0) {
		form_focus(form, tag);
		message_box("�訡��", "�訡�� ���������� TLV", dlg_yes, 0, al_center);
		form_draw(form);
		return ret;
	}
	return 0;
}

static int fa_tlv_add_cashier(form_t *form, bool save) {
	form_data_t cashier;
	form_data_t post;
	char result[64+1];

	form_get_data(form, 1021, 1, &cashier);
	form_get_data(form, 9999, 1, &post);
	size_t length = cashier.size;

	if (length == 0) {
		form_focus(form, 1021);
		message_box("�訡��", "��易⥫쭮� ���� �� ���������", dlg_yes, 0, al_center);
		form_draw(form);
		return -1;
	}

	memcpy(result, cashier.data, cashier.size);
	if (post.size > 0 && cashier.size < 63) {
		size_t l = 64 - cashier.size - 1;
		l = MIN(l, post.size);

		result[cashier.size] = ' ';
		memcpy(result + cashier.size + 1, post.data, l);
		length += l + 1;
	}
	result[length] = 0;

	if (save) {
		form_data_t inn;
		form_get_data(form, 1203, 0, &inn);
		FILE *f = fopen("/home/sterm/cashier.txt", "w");
		if (f != NULL) {
			fprintf(f, "%s\n%s\n%s\n%s\n",
			(const char *)cashier.data,
			(const char *)post.data,
			(const char *)inn.data,
			(const char *)result);
			fclose(f);
		} else {
			message_box("�訡��", "�訡�� ����� ��ࠬ��஢ ������ ᬥ�� � 䠩�",
				dlg_yes, 0, al_center);
			form_draw(form);
			return -1;
		}
	}

	return ffd_tlv_add_string(1021, result);
}

typedef struct {
    char cashier[64+1]; // �����
    char post[64+1]; // ���������
    char cashier_inn[12+1]; // ��� �����
    char cashier_post[64+1]; //  ����� + ���������
} cashier_data_t;

static int fa_load_cashier_data(cashier_data_t *data) {
	FILE *f = fopen("/home/sterm/cashier.txt", "r");
	if (f != NULL) {
		char *strings[] = { data->cashier, data->post, data->cashier_inn, data->cashier_post };
		size_t sizes[] = { sizeof(data->cashier), sizeof(data->post),
			sizeof(data->cashier_inn), sizeof(data->cashier_post) } ;

		for (int i = 0; i < ASIZE(strings); i++) {
			char *s;
			if ((s = fgets(strings[i], sizes[i], f)) == NULL)
				strings[i][0] = 0;
			int len = strlen(s);
			if (s[len - 1] == '\n')
				s[len - 1] = 0;
		}
		fclose(f);

		return 0;
	}

	return 0;
}

typedef void (*update_screen_func_t)(void *arg);

static bool fa_create_doc(uint16_t doc_type, const uint8_t *pattern_footer,
		size_t pattern_footer_size, 
		update_screen_func_t update_func, void *update_func_arg) {
	uint8_t status;

	if ((status = fd_create_doc(doc_type, pattern_footer, pattern_footer_size)) != 0) {
		if (status == 0x46) {
			struct kkt_last_doc_info ldi;
			uint8_t err_info[32];
			size_t err_info_len;

			printf("#1 %d\n", doc_type);
LCheckLastDocNo:
			status = kkt_get_last_doc_info(&ldi, err_info, &err_info_len);
			if (status != 0) {
				printf("#2: %d\n", status);
				fd_set_error(status, err_info, err_info_len);
			} else {
				printf("#3: %d, %d\n", ldi.last_nr, ldi.last_printed_nr);
				if (ldi.last_nr != ldi.last_printed_nr) {
					if (message_box("�訡��", "��᫥���� ��ନ஢���� ���㬥�� �� �� �����⠭.\n"
							"��� ��� ���� ��⠢�� �㬠�� � ��� � ������ ������ \"��\"",
							dlg_yes_no, 0, al_center) == DLG_BTN_YES) {
						if (update_func)
							update_func(update_func_arg);
						status = fd_print_last_doc(ldi.last_type);
						if (status != 0) {
							fd_set_error(status, err_info, err_info_len);
						} else
							return false;
					} else {
						if (update_func)
							update_func(update_func_arg);
						return false;
					}
				}
			}
		}
	
		const char *error;
		fd_get_last_error(&error);
		message_box("�訡��", error, dlg_yes, 0, al_center);
		if (update_func)
			update_func(update_func_arg);

		if (status == 0x41 || status == 0x42 || status == 0x44)
			goto LCheckLastDocNo;

		return false;
	}
	return true;
}

static void update_form(void *form) {
	if (form)
		form_draw((form_t *)form);
}

static void update_cheque(void *arg) {
	kbd_flush_queue();
	cheque_draw();
}

void fa_open_shift() {
	BEGIN_FORM(open_shift_form, "����⨥ ᬥ��")
		FORM_ITEM_EDIT_TEXT(1021, "�����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "��������� �����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "��� �����:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()
	form_t *form = open_shift_form;

	while (form_execute(form) == 1) {
		ffd_tlv_reset();
		if (fa_tlv_add_cashier(form, true) != 0 ||
			fa_tlv_add_fixed_string(form, 1203, 12, false) != 0)
			continue;

		if (fa_create_doc(OPEN_SHIFT, NULL, 0, update_form, form))
			break;
	}

	fa_set_group(FAPP_GROUP_MENU);
}

void fa_close_shift() {
	BEGIN_FORM(close_shift_form, "�����⨥ ᬥ��")
		FORM_ITEM_EDIT_TEXT(1021, "�����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "��������� �����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "��� �����:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()
	form_t *form = close_shift_form;

	cashier_data_t data;
	if (fa_load_cashier_data(&data) == 0) {
		FORM_EDIT_TEXT_SET_TEXT(form, 1021, data.cashier, strlen(data.cashier));
		FORM_EDIT_TEXT_SET_TEXT(form, 9999, data.post, strlen(data.post));
		FORM_EDIT_TEXT_SET_TEXT(form, 1203, data.cashier_inn, strlen(data.cashier_inn));
	}

	while (form_execute(form) == 1) {
		ffd_tlv_reset();
		if (fa_tlv_add_cashier(form, false) != 0 ||
			fa_tlv_add_fixed_string(form, 1203, 12, false) != 0)
			continue;

		if (fa_create_doc(CLOSE_SHIFT, NULL, 0, update_form, form))
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
		ffd_tlv_reset();
		if (fa_create_doc(CALC_REPORT, NULL, 0, update_form, form))
			break;
	}
	form_destroy(form);
	fa_set_group(FAPP_GROUP_MENU);
}

void fa_close_fs() {
	BEGIN_FORM(close_fs_form, "�����⨥ ��")
		FORM_ITEM_EDIT_TEXT(1021, "�����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "��� �����:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()
	form_t *form = close_fs_form;

	while (form_execute(form) == 1) {
		ffd_tlv_reset();
		if (fa_tlv_add_string(form, 1021, true) != 0 ||
			fa_tlv_add_fixed_string(form, 1203, 12, false) != 0)
			continue;

		if (fa_create_doc(CLOSE_FS, NULL, 0, update_form, form))
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

static int fa_fill_registration_tlv(form_t *form) {
	ffd_tlv_reset();

	int tax_systems = form_get_int_data(form, 1062, 0, 0);
	int reg_modes = form_get_int_data(form, 9999, 0, 0);

	if (fa_tlv_add_string(form, 1048, true) != 0 ||
		fa_tlv_add_fixed_string(form, 1018, 12, true) != 0 ||
		fa_tlv_add_string(form, 1009, false) != 0 ||
		fa_tlv_add_string(form, 1187, false) != 0 ||
		ffd_tlv_add_uint8(1062, (uint8_t)tax_systems) != 0 ||
		fa_tlv_add_fixed_string(form, 1037, 20, true) != 0 ||
		fa_tlv_add_string(form, 1036, (reg_modes & REG_MODE_AUTOMAT) != 0) != 0 ||
		fa_tlv_add_string(form, 1021, true) != 0 ||
		fa_tlv_add_fixed_string(form, 1203, 12, false) != 0 ||
		fa_tlv_add_string(form, 1060, (reg_modes & REG_MODE_OFFLINE) == 0) != 0 ||
		fa_tlv_add_string(form, 1117, (reg_modes & REG_MODE_OFFLINE) == 0) != 0 ||
		fa_tlv_add_string(form, 1046, (reg_modes & REG_MODE_OFFLINE) == 0) != 0 ||
		fa_tlv_add_fixed_string(form, 1017, 12, (reg_modes & REG_MODE_OFFLINE) == 0) != 0) {
		return -1;
	}

	uint16_t reg_mode_tags[] = { 1056, 1002, 1001, 1109, 1110, 1108 };
	for (int i = 0; i < ASIZE(reg_mode_tags); i++)
		if ((reg_modes & (1 << i)) != 0)
			ffd_tlv_add_uint8(reg_mode_tags[i], 1);
	return 0;
}

void fa_registration() {
	BEGIN_FORM(reg_form, "���������")
		FORM_ITEM_EDIT_TEXT(1048, "������������ ���짮��⥫�:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1018, "��� ���짮��⥫�:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1009, "���� ���⮢:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1187, "���� ���⮢:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_BITSET(1062, "���⥬� ���������������:", tax_modes, tax_modes, 5, 0)
		FORM_ITEM_EDIT_TEXT(1037, "�������樮��� ����� ���:", NULL, FORM_INPUT_TYPE_NUMBER, 16)
		FORM_ITEM_BITSET(9999, "������ ࠡ���:", short_modes, modes, 6, 0)
		FORM_ITEM_EDIT_TEXT(1036, "����� ��⮬��:", NULL, FORM_INPUT_TYPE_TEXT, 20)
		FORM_ITEM_EDIT_TEXT(1021, "�����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "��� �����:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1060, "���� ᠩ� ���:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1117, "���� �. ����� ���. 祪�:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1046, "������������ ���:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1017, "��� ���:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()
	form_t *form = reg_form;

	while (form_execute(form) == 1) {
		if (fa_fill_registration_tlv(form) != 0)
			continue;

		if (fa_create_doc(REGISTRATION, NULL, 0, update_form, form))
			break;
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

static int fa_fill_reregistration_form(form_t *form) {
	uint8_t modes = 0;

	if (rereg_data_len > 0) {
		for (const ffd_tlv_t *tlv = (ffd_tlv_t *)rereg_data,
				*end = (ffd_tlv_t *)(rereg_data + rereg_data_len);
				tlv < end;
				tlv = FFD_TLV_NEXT(tlv)) {
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
	return 0;
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
		FORM_ITEM_EDIT_TEXT(1036, "����� ��⮬��:", NULL, FORM_INPUT_TYPE_TEXT, 20)
		FORM_ITEM_EDIT_TEXT(1021, "�����:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "��� �����:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1060, "���� ᠩ� ���:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1117, "���� �. ����� ���. 祪�:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1046, "������������ ���:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1017, "��� ���:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BITSET(9998, "��稭� ���ॣ����樨", short_rereg_reason, rereg_reason, 4, 0)
		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()
	form_t *form = rereg_form;

	if (empty)
		fa_fill_reregistration_form(form);

	while (form_execute(form) == 1) {
		if (fa_fill_registration_tlv(form) != 0) {
			printf("���ࠢ��쭮� ���������� �����\n");
			continue;
		}

		int rereg_reason = form_get_int_data(form, 9998, 0, 0);
		printf("rereg_reason = %d\n", rereg_reason);
		if (rereg_reason == 0) {
			message_box("�訡��", "�� 㪠���� �� ���� ��稭� ���ॣ����樨", dlg_yes, 0, al_center);
			form_focus(form, 9998);
			form_draw(form);
			continue;
		}

		if (fa_create_doc(RE_REGISTRATION, NULL, 0, update_form, form))
			break;
	}
	fa_set_group(FAPP_GROUP_MENU);
}

static int fa_get_int_field(form_t *form, uint16_t tag) {
	int ret = form_get_int_data(form, tag, 0, -1);
	if (ret == -1) {
		message_box("�訡��", "���祭�� ��易⥫쭮�� ���� �� 㪠����", dlg_yes, 0, al_center);
		form_focus(form, tag);
		form_draw(form);
		return -1;
	}
	return ret;
}

void fa_cheque_corr() {
	const char *str_pay_type[] = { "���४�� ��室�", "���४�� ��室�" };
	const char *str_tax_mode[] = { "���", "��� �����", "��� �����-������", "����", "����" };
	const char *str_corr_type[] = { "�������⥫쭮", "�� �।��ᠭ��" };

	BEGIN_FORM(cheque_corr_form, "��� ���४樨")
		FORM_ITEM_LISTBOX(1054, "�ਧ��� ����:", str_pay_type, ASIZE(str_pay_type), -1)
		FORM_ITEM_LISTBOX(1055, "���⥬� ���������������:", str_tax_mode, ASIZE(str_tax_mode), -1)
		FORM_ITEM_LISTBOX(1173, "��� ���४樨:", str_corr_type, ASIZE(str_corr_type), -1)
		FORM_ITEM_EDIT_TEXT(1177, "���ᠭ�� ���४樨:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1178, "��� ���४樨:", NULL, FORM_INPUT_TYPE_DATE, 10)
		FORM_ITEM_EDIT_TEXT(1179, "����� �।��ᠭ��:", NULL, FORM_INPUT_TYPE_TEXT, 32)

		FORM_ITEM_EDIT_TEXT(1031, "�����묨:", "0", FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1081, "��������묨:", "0", FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1215, "�।����⮩:", "0", FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1216, "���⮯��⮩:", "0", FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1217, "������ �।�⠢������:", "0", FORM_INPUT_TYPE_MONEY, 16)

		FORM_ITEM_EDIT_TEXT(1102, "��� 20%:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1103, "��� 10%:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1104, "��� 0%:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1105, "��� ���:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1106, "��� 20/120:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1107, "��� 10/110:", NULL, FORM_INPUT_TYPE_MONEY, 16)

		FORM_ITEM_BUTTON(1, "�����", NULL)
		FORM_ITEM_BUTTON(0, "�⬥��", NULL)
	END_FORM()
	form_t *form = cheque_corr_form;

	while (form_execute(form) == 1) {
		ffd_tlv_reset();

		int pay_type;
		int tax_system;
		int corr_type;

		if ((pay_type = fa_get_int_field(form, 1054)) < 0 ||
			(tax_system = fa_get_int_field(form, 1055)) < 0 ||
			(corr_type = fa_get_int_field(form, 1173)) < 0)
			continue;

		if (ffd_tlv_add_uint8(1054, (pay_type == 0) ? 1 : 3) != 0 ||
			ffd_tlv_add_uint8(1055, (1 << tax_system)) != 0 ||
			ffd_tlv_add_uint8(1173, corr_type) != 0) 
			continue;
		if (ffd_tlv_stlv_begin(1174, 292) != 0 ||
			fa_tlv_add_string(form, 1177, false) != 0 ||
			fa_tlv_add_unixtime(form, 1178, true) != 0 ||
			fa_tlv_add_string(form, 1179, true) != 0 ||
			ffd_tlv_stlv_end() != 0)
			continue;

		if (fa_tlv_add_vln(form, 1031, true) != 0 ||
			fa_tlv_add_vln(form, 1081, true) != 0 ||
			fa_tlv_add_vln(form, 1215, true) != 0 ||
			fa_tlv_add_vln(form, 1216, true) != 0 ||
			fa_tlv_add_vln(form, 1217, true) != 0 ||

			fa_tlv_add_vln(form, 1102, false) != 0 ||
			fa_tlv_add_vln(form, 1103, false) != 0 ||
			fa_tlv_add_vln(form, 1104, false) != 0 ||
			fa_tlv_add_vln(form, 1105, false) != 0 ||
			fa_tlv_add_vln(form, 1106, false) != 0 ||
			fa_tlv_add_vln(form, 1107, false) != 0)
			continue;

		cashier_data_t data = { "", "", "", "" };
		fa_load_cashier_data(&data);
		if (data.cashier_post[0])
			ffd_tlv_add_string(1021, data.cashier_post);
		if (data.cashier_inn[0])
			ffd_tlv_add_fixed_string(1203, data.cashier_inn, 12);

		if (fa_create_doc(CHEQUE_CORR, NULL, 0, update_form, form))
			break;
	}
	fa_set_group(FAPP_GROUP_MENU);
}

static size_t get_phone(char *src, char *dst) {
	if (src == NULL)
		return 0;
	char ch = *src;
	size_t len = 0;
	if (ch == '8') {
		*dst++ = '+';
		*dst++ = '7';
		len += 2;
		src++;
	} else if (isdigit(ch)) {
		*dst++ = '+';
		len++;
	}

	while (*src) {
		*dst++ = *src++;
		len++;
	}
	*dst = 0;

	return len;
}

void fa_cheque() {
	int64_t sumN = 0;
	int64_t sumE = 0;
	int64_t sumI = 0;
	int64_t sumC = 0;

	for (list_item_t *li1 = _ad->clist.head; li1 != NULL; li1 = li1->next) {
		C *c = LIST_ITEM(li1, C);
		if (c->t1054 == 1 || c->t1054 == 4) {
			sumN += c->sum.n;
			sumE += c->sum.e;
		} else {
			sumN -= c->sum.n;
			sumE -= c->sum.e;
		}
	}
	int64_t sn = llabs(sumN);

	cheque_init();

	while (true) {
		if (cheque_execute()) {
		
			if (sumN > 0) {
				form_t *form = NULL;
				char title[256];
				
				sprintf(title, "����� ᤠ� (%s: %.1lld.%.2lld ���)",
					sumN > 0 ? "����室��� ������� �� ���ᠦ��" : 
					"����室��� �뤠�� ���ᠦ���",
					sn / 100, sn % 100);
					
		
				BEGIN_FORM(form, title)
					FORM_ITEM_EDIT_TEXT(1031, "������ �㬬�, �ਭ���� �� ���ᠦ��:", "", FORM_INPUT_TYPE_MONEY, 16)
					FORM_ITEM_BUTTON(1, "��", NULL)
					FORM_ITEM_BUTTON(0, "�⬥��", NULL)
				END_FORM()
				
				while (1) {
					int64_t n;
					if (form_execute(form) == 1) {
						form_data_t data;
						form_get_data(form, 1031, 1, &data);
						n = form_data_to_vln(&data);
						sumC = n - sumN;
					} else
						break;
						
					printf("sumI: %lld, sumN: %lld\n", sumI, sumN);
			
					if (n < sumN) {
						message_box("�訡��", "�㬬� �ਭ���� �� ���ᠦ�� ����� \n"
							"�� ������ ���� ����� �㬬� ��� ������ �����묨", dlg_yes, 0, al_center);
						kbd_flush_queue();
						form_draw(form);
					} else {
						char buffer[1024];
						sprintf(buffer, "�����, ���������� �� ���������: %.1lld.%.2lld\n"
							"%s: %.1lld.%.2lld\n"
							"�����: %.1lld.%.2lld\n"
							"������ 祪(�)?",
							n / 100, n % 100,
							sumN > 0 ? "���������� �������� ��� �����\x9b" : "����� ��� �\x9b����",
							sn / 100, sn % 100,
							sumC / 100, sumC % 100);
						if (message_box("�����������", buffer, dlg_yes_no, 0, al_center) == DLG_BTN_YES) {
							kbd_flush_queue();
							sumI = n;
							break;
						} else {
							form_draw(form);
							continue;
						}
					}
				}
				form_destroy(form);
				
				if (sumI < sumN) {
					cheque_draw();
					continue;
				}
			}
		
		
			fdo_suspend();
			
			cashier_data_t data = { "", "", "", "" };
			fa_load_cashier_data(&data);

			while (_ad->clist.head) {
				C *c = LIST_ITEM(_ad->clist.head, C);
				ffd_tlv_reset();

				if (data.cashier_post[0])
					ffd_tlv_add_string(1021, data.cashier_post);
				printf("CASHIER_INN: %d\n", data.cashier_inn[0]);
				if (data.cashier_inn[0])
					ffd_tlv_add_fixed_string(1203, data.cashier_inn, 12);
					
				ffd_tlv_add_uint8(1054, c->t1054);
				ffd_tlv_add_uint8(1055, c->t1055);
				if (c->pe)
					ffd_tlv_add_string(1008, c->pe);
				ffd_tlv_add_vln(1031, (uint64_t)c->sum.n);
				ffd_tlv_add_vln(1081, (uint64_t)c->sum.e);
				ffd_tlv_add_vln(1215, (uint64_t)c->sum.p);
				ffd_tlv_add_vln(1216, 0);
				ffd_tlv_add_vln(1217, 0);

				if (c->p != user_inn) {
					ffd_tlv_add_uint8(1057, 1 << 6);
					char phone[19+1];
					/*size_t size =*/ get_phone(c->h, phone);

					ffd_tlv_add_string(1171, phone);
				}

				if (c->t1086 != NULL) {
					ffd_tlv_stlv_begin(1084, 320);
					ffd_tlv_add_string(1085, "��������");
					ffd_tlv_add_string(1086, c->t1086);
					ffd_tlv_stlv_end();
				}
				for (list_item_t *i1 = c->klist.head; i1; i1 = i1->next) {
					K *k = LIST_ITEM(i1, K);
					for (list_item_t *i2 = k->llist.head; i2; i2 = i2->next) {
						L *l = LIST_ITEM(i2, L);
						ffd_tlv_stlv_begin(1059, 1024);
						ffd_tlv_add_uint8(1214, l->r);
						char s1030[256];
						sprintf(s1030, "%s\n\r���㬥�� \xfc%lld", l->s, k->d);
						ffd_tlv_add_string(1030, s1030);
						ffd_tlv_add_vln(1079, l->t);
						ffd_tlv_add_fvln(1023, 1, 0);
						if (l->n > 0)
							ffd_tlv_add_vln(1199, l->n);
						if (l->n >= 1 && l->n <= 4) {
							ffd_tlv_add_vln(1198, l->c);
							ffd_tlv_add_vln(1200, l->c);
						}
						if (c->p != user_inn) {
							char inn[12+1];
							if (c->p > 9999999999ll)
								sprintf(inn, "%.12lld", c->p);
							else
								sprintf(inn, "%.10lld", c->p);
							ffd_tlv_add_fixed_string(1226, inn, 12);
						}
						ffd_tlv_stlv_end();
					}
				}

				uint8_t* pattern_footer = NULL;
				size_t pattern_footer_size = 0;
				uint8_t pattern_footer_buffer[1024] = { 0 };

				if (_ad->clist.count == 1) {
					size_t n = 0;
					pattern_footer = pattern_footer_buffer;
					char *p = (char *)pattern_footer_buffer;
					if (sumN != 0) {
						n += sprintf(p + n, "������ ������\x9b��\r\n");
						if (sumN > 0)
							n += sprintf(p + n, "����� � ������: \x5%.1lld.%.2lld ���.\r\n",
								sumN / 100, sumN % 100);
						else {
							sumN = -sumN;
							n += sprintf(p + n, "����� � �\x9b�����: \x5%.1lld.%.2lld ���.\r\n",
								sumN / 100, sumN % 100);
						}
						if (sumI > 0) {
							n += sprintf(p + n, "��������: \x5%.1lld.%.2lld ���.\r\n",
								sumI / 100, sumI % 100);
							n += sprintf(p + n, "�����: \x5%.1lld.%.2lld ���.\r\n",
								sumC / 100, sumC % 100);
						}
					}

					if (sumE != 0) {
						n += sprintf(p + n, "������ ���������\x9b��\r\n");
						if (sumE > 0)
							n += sprintf(p + n, "����� � ������: \x5%.1lld.%.2lld ���.\r\n",
								sumE / 100, sumE % 100);
						else
							n += sprintf(p + n, "����� � �\x9b�����: \x5%.1lld.%.2lld ���.\r\n",
								sumE / 100, sumE % 100);
					}

					pattern_footer_size = n;
				}

				if (fa_create_doc(CHEQUE, pattern_footer, pattern_footer_size, update_cheque, NULL)) {
					list_remove(&_ad->clist, c);
					AD_save();
					cheque_draw();
				} else
					break;
				/*else {
					kbd_flush_queue();
					cheque_draw();
					break;
				}*/
			}
			fdo_resume();

			if (!_ad->clist.head)
				break;
		} else
			break;
	}

	fa_set_group(FAPP_GROUP_MENU);
}

static bool process_fa_cmd(int cmd) {
	bool ret = true;
	switch (cmd){
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
		case cmd_cheque_fa:
			fa_cheque();
			break;
		default:
			ret = false;
			break;
	}
	return ret;
}


bool process_fa(const struct kbd_event *e)
{
	if (fa_arg != cmd_fa)
		return false;
	if (!e->pressed)
		return true;
	if (fa_active_group == FAPP_GROUP_MENU) {
		if (process_menu(fa_menu, (struct kbd_event *)e) != cmd_none)
			return process_fa_cmd(get_menu_command(fa_menu));
    }
	return true;
}
