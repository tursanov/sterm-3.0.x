/* Настройка параметров терминала. (c) gsr & alex 2000-2004, 2018. */

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
#include "kkt/fd/tlv.h"

static int fa_active_group = -1;
static int fa_active_item = -1;
//static int fa_active_control = -1;
int fa_cm = cmd_none;
static struct menu *fa_menu = NULL;

/* Группы параметров настройки */
enum {
	FAPP_GROUP_MENU = -1,
};

/* Работа с меню настроек */
static bool fa_create_menu(void)
{
	fa_menu = new_menu(false, false);
	add_menu_item(fa_menu, new_menu_item("Регистрация", cmd_reg_fa, true));
	add_menu_item(fa_menu, new_menu_item("Перерегистрация", cmd_rereg_fa, true));
	add_menu_item(fa_menu, new_menu_item("Открытие смены", cmd_open_shift_fa, true));
	add_menu_item(fa_menu, new_menu_item("Закрытие смены", cmd_close_shift_fa, true));
	add_menu_item(fa_menu, new_menu_item("Чек", cmd_cheque_fa, true));
	add_menu_item(fa_menu, new_menu_item("Чек коррекции", cmd_cheque_corr_fa, true));
	add_menu_item(fa_menu, new_menu_item("Отчет о текущeм состоянии расчетов", cmd_calc_report_fa, true));
	add_menu_item(fa_menu, new_menu_item("Закрытие ФН", cmd_close_fs_fa, true));
	add_menu_item(fa_menu, new_menu_item("Выход", cmd_exit, true));
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

	fa_get_reregistration_data();

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

static int fa_tlv_add_string(form_t *form, uint16_t tag, int max_length, bool fixed, bool required) {
	form_data_t data;

	if (!form_get_data(form, tag, 1, &data)) {
		form_focus(form, tag);
		message_box("Ошибка", "Указан неверный тэг", dlg_yes, 0, al_center);
		form_draw(form);
		return -1;
	}

	if (data.size == 0) {
		if (required) {
			form_focus(form, tag);
			message_box("Ошибка", "Обязательное поле не заполнено", dlg_yes, 0, al_center);
			form_draw(form);
			return -2;
		}
		return 0;
	}

	int ret;
	if ((ret = ffd_tlv_add_string(tag, (const char *)data.data, max_length, fixed)) != 0) {
		form_focus(form, tag);
		message_box("Ошибка", "Ошибка при добавлении TLV. Обратитесь к разработчикам", dlg_yes, 0, al_center);
		form_draw(form);
		return ret;
	}
	return 0;
}

static int fa_tlv_add_vln(form_t *form, uint16_t tag, bool required) {
	form_data_t data;

	if (!form_get_data(form, tag, 1, &data)) {
		form_focus(form, tag);
		message_box("Ошибка", "Указан неверный тэг", dlg_yes, 0, al_center);
		form_draw(form);
		return -1;
	}

	if (data.size == 0) {
		if (required) {
			form_focus(form, tag);
			message_box("Ошибка", "Обязательное поле не заполнено", dlg_yes, 0, al_center);
			form_draw(form);
			return -2;
		}
		return 0;
	}

	uint64_t value = 0;
	uint64_t d = 1;
	const char *text = (const char *)data.data + data.size - 1;
	for (int i = 0; i < data.size; i++, text--, d = d * 10) {
		if (isdigit(*text))
			value += (*text - '0') * d;
	}

	int ret;
	if ((ret = ffd_tlv_add_vln(tag, value)) != 0) {
		form_focus(form, tag);
		message_box("Ошибка", "Неправильное значение", dlg_yes, 0, al_center);
		form_draw(form);
		return ret;
	}
	return 0;
}

static int fa_tlv_add_unixtime(form_t *form, uint16_t tag, bool required) {
	form_data_t data;

	if (!form_get_data(form, tag, 1, &data)) {
		form_focus(form, tag);
		message_box("Ошибка", "Указан неверный тэг", dlg_yes, 0, al_center);
		form_draw(form);
		return -1;
	}

	if (data.size == 0) {
		if (required) {
			form_focus(form, tag);
			message_box("Ошибка", "Обязательное поле не заполнено", dlg_yes, 0, al_center);
			form_draw(form);
			return -2;
		}
		return 0;
	}

	struct tm tm;
	char *s;
	if ((s = strptime((const char *)data.data, "%d.%m.%Y", &tm)) == NULL || *s != 0) {
		form_focus(form, tag);
		message_box("Ошибка", "Неправильное значение", dlg_yes, 0, al_center);
		form_draw(form);
		return -1;
	}

	time_t value = timelocal(&tm);

	int ret;
	if ((ret = ffd_tlv_add_unix_time(tag, value)) != 0) {
		form_focus(form, tag);
		message_box("Ошибка", "Ошибка добавления TLV", dlg_yes, 0, al_center);
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
		message_box("Ошибка", "Обязательное поле не заполнено", dlg_yes, 0, al_center);
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
			message_box("Ошибка", "Ошибка записи параметров открытия смены в файл",
				dlg_yes, 0, al_center);
			form_draw(form);
			return -1;
		}
	}

	return ffd_tlv_add_string(1021, result, length, false);
}

typedef struct {
    char cashier[64+1]; // кассир
    char post[64+1]; // должность
    char cashier_inn[12+1]; // инн кассира
    char cashier_post[64+1]; //  кассир + должность
} cashier_data_t;

static int fa_load_cashier_data(cashier_data_t *data) {
	FILE *f = fopen("/home/sterm/cashier.txt", "r");
	if (f != NULL) {
		char *s[] = {
			data->cashier,
			data->post,
			data->cashier_inn,
			data->cashier_post
		};
		int l[] = {
			sizeof(data->cashier) - 1,
			sizeof(data->post),
			sizeof(data->cashier_inn),
			sizeof(data->cashier_post)
		};

		memset(data, 0 ,sizeof(data));
		for (int i = 0; i < 4; i++) {
			if (!fgets(s[i], l[i], f))
				break;
		}
		fclose(f);
		return 0;
	}

	return 0;
}

void fa_open_shift() {
	BEGIN_FORM(open_shift_form, "Открытие смены")
		FORM_ITEM_EDIT_TEXT(1021, "Кассир:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "Должность кассира:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "ИНН Кассира:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "Печать", NULL)
		FORM_ITEM_BUTTON(0, "Отмена", NULL)
	END_FORM()
	form_t *form = open_shift_form;

	while (form_execute(form) == 1) {
		ffd_tlv_reset();
		if (fa_tlv_add_cashier(form, true) != 0 ||
			fa_tlv_add_string(form, 1203, 12, true, false) != 0)
			continue;

		if (fd_create_doc(OPEN_SHIFT, NULL, 0) != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("Ошибка", error, dlg_yes, 0, al_center);
			form_draw(form);
		} else
			break;
	}

	fa_set_group(FAPP_GROUP_MENU);
}

void fa_close_shift() {
	BEGIN_FORM(close_shift_form, "Закрытие смены")
		FORM_ITEM_EDIT_TEXT(1021, "Кассир:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(9999, "Должность кассира:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "ИНН Кассира:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "Печать", NULL)
		FORM_ITEM_BUTTON(0, "Отмена", NULL)
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
			fa_tlv_add_string(form, 1203, 12, true, false) != 0)
			continue;

		if (fd_create_doc(CLOSE_SHIFT, NULL, 0) != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("Ошибка", error, dlg_yes, 0, al_center);
			form_draw(form);
		} else
			break;
	}
	fa_set_group(FAPP_GROUP_MENU);
}

void fa_calc_report() {
	form_t *form = NULL;
	BEGIN_FORM(form, "Отчет о текущем состоянии расчетов")
		FORM_ITEM_BUTTON(1, "Печать", NULL)
		FORM_ITEM_BUTTON(0, "Отмена", NULL)
	END_FORM()

	while (form_execute(form) == 1) {
		ffd_tlv_reset();
		if (fd_create_doc(CALC_REPORT, NULL, 0) != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("Ошибка", error, dlg_yes, 0, al_center);
			form_draw(form);
		} else
			break;
	}
	form_destroy(form);
	fa_set_group(FAPP_GROUP_MENU);
}

void fa_close_fs() {
	BEGIN_FORM(close_fs_form, "Закрытие ФН")
		FORM_ITEM_EDIT_TEXT(1021, "Кассир:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "ИНН Кассира:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "Печать", NULL)
		FORM_ITEM_BUTTON(0, "Отмена", NULL)
	END_FORM()
	form_t *form = close_fs_form;

	while (form_execute(form) == 1) {
		ffd_tlv_reset();
		if (fa_tlv_add_string(form, 1021, 64, false, true) != 0 ||
			fa_tlv_add_string(form, 1203, 12, true, false) != 0)
			continue;

		if (fd_create_doc(CLOSE_FS, NULL, 0) != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("Ошибка", error, dlg_yes, 0, al_center);
			form_draw(form);
		} else
			break;
	}
	fa_set_group(FAPP_GROUP_MENU);
}

static const char *tax_modes[8] = { "ОСН", "УСН ДОХОД", "УСН ДОХОД-РАСХОД", "ЕНВД", "ЕСХН",
		NULL, NULL, NULL };
static const char *short_modes[8] = { "ШФД", "АВТОН.Р.", "АВТОМАТ.Р.",
		"УСЛУГИ", "БСО", "ИНТЕРНЕТ", NULL, NULL };
static const char *modes[8] = { "Шифрование", "Автономный режим", "Автоматический режим",
		"Применение в сфере услуг", "Режим БСО", "Применение в Интернет", NULL, NULL };

static int fa_fill_registration_tlv(form_t *form) {
	ffd_tlv_reset();

	int tax_systems = form_get_int_data(form, 1062, 0, 0);
	int reg_modes = form_get_int_data(form, 9999, 0, 0);

	if (fa_tlv_add_string(form, 1048, 256, false, true) != 0 ||
		fa_tlv_add_string(form, 1018, 12, true, true) != 0 ||
		fa_tlv_add_string(form, 1009, 256, true, false) != 0 ||
		fa_tlv_add_string(form, 1187, 256, true, false) != 0 ||
		ffd_tlv_add_uint8(1062, (uint8_t)tax_systems) != 0 ||
		fa_tlv_add_string(form, 1037, 20, true, true) != 0 ||
		fa_tlv_add_string(form, 1036, 20, false, (reg_modes & REG_MODE_AUTOMAT) != 0) != 0 ||
		fa_tlv_add_string(form, 1021, 64, false, true) != 0 ||
		fa_tlv_add_string(form, 1203, 12, true, false) != 0 ||
		fa_tlv_add_string(form, 1060, 256, false, (reg_modes & REG_MODE_OFFLINE) == 0) != 0 ||
		fa_tlv_add_string(form, 1117, 64, false, (reg_modes & REG_MODE_OFFLINE) == 0) != 0 ||
		fa_tlv_add_string(form, 1046, 256, false, (reg_modes & REG_MODE_OFFLINE) == 0) != 0 ||
		fa_tlv_add_string(form, 1017, 12, true, (reg_modes & REG_MODE_OFFLINE) == 0) != 0) {
		return -1;
	}

	uint16_t reg_mode_tags[] = { 1056, 1002, 1001, 1109, 1110, 1108 };
	for (int i = 0; i < ASIZE(reg_mode_tags); i++)
		if ((reg_modes & (1 << i)) != 0)
			ffd_tlv_add_uint8(reg_mode_tags[i], 1);
	return 0;
}

void fa_registration() {
	BEGIN_FORM(reg_form, "Регистрация")
		FORM_ITEM_EDIT_TEXT(1048, "Наименование пользователя:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1018, "ИНН пользователя:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1009, "Адрес расчетов:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1187, "Место расчетов:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_BITSET(1062, "Системы налогообложения:", tax_modes, tax_modes, 5, 0)
		FORM_ITEM_EDIT_TEXT(1037, "Регистрационный номер ККТ:", NULL, FORM_INPUT_TYPE_NUMBER, 16)
		FORM_ITEM_BITSET(9999, "Режимы работы:", short_modes, modes, 6, 0)
		FORM_ITEM_EDIT_TEXT(1036, "Номер автомата:", NULL, FORM_INPUT_TYPE_TEXT, 20)
		FORM_ITEM_EDIT_TEXT(1021, "Кассир:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "ИНН Кассира:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1060, "Адрес сайта ФНС:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1117, "Адрес эл. почты отпр. чека:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1046, "Наименование ОФД:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1017, "ИНН ОФД:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BUTTON(1, "Печать", NULL)
		FORM_ITEM_BUTTON(0, "Отмена", NULL)
	END_FORM()
	form_t *form = reg_form;

	while (form_execute(form) == 1) {
		if (fa_fill_registration_tlv(form) != 0)
			continue;

		if (fd_create_doc(REGISTRATION, NULL, 0) != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("Ошибка", error, dlg_yes, 0, al_center);
			form_draw(form);
		} else
			break;
	}
	fa_set_group(FAPP_GROUP_MENU);
}

static const char *short_rereg_reason[8] = { "ЗАМЕНА ФН", "ЗАМЕНА ОФД", "ИЗМ.РЕКВ.",
	"ИЗМ.НАСТР.ККТ", NULL,  NULL, NULL, NULL };
static const char *rereg_reason[8] = { "Замена ФН", "Замена ОФД", "Изменение реквизитов",
	"Изменение настроек ККТ", NULL,  NULL, NULL, NULL };

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
	BEGIN_FORM(rereg_form, "Перерегистрация")
		FORM_ITEM_EDIT_TEXT(1048, "Наименование пользователя:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1018, "ИНН пользователя:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1009, "Адрес расчетов:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1187, "Место расчетов:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_BITSET(1062, "Системы налогообложения:", tax_modes, tax_modes, 5, 0)
		FORM_ITEM_EDIT_TEXT(1037, "Регистрационный номер ККТ:", NULL, FORM_INPUT_TYPE_NUMBER, 16)
		FORM_ITEM_BITSET(9999, "Режимы работы:", short_modes, modes, 6, 0)
		FORM_ITEM_EDIT_TEXT(1036, "Номер автомата:", NULL, FORM_INPUT_TYPE_TEXT, 20)
		FORM_ITEM_EDIT_TEXT(1021, "Кассир:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1203, "ИНН Кассира:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_EDIT_TEXT(1060, "Адрес сайта ФНС:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1117, "Адрес эл. почты отпр. чека:", NULL, FORM_INPUT_TYPE_TEXT, 64)
		FORM_ITEM_EDIT_TEXT(1046, "Наименование ОФД:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1017, "ИНН ОФД:", NULL, FORM_INPUT_TYPE_NUMBER, 12)
		FORM_ITEM_BITSET(9998, "Причины перерегистрации", short_rereg_reason, rereg_reason, 4, 0)
		FORM_ITEM_BUTTON(1, "Печать", NULL)
		FORM_ITEM_BUTTON(0, "Отмена", NULL)
	END_FORM()
	form_t *form = rereg_form;

	if (empty)
		fa_fill_reregistration_form(form);

	while (form_execute(form) == 1) {
		if (fa_fill_registration_tlv(form) != 0) {
			printf("Неправильное заполнение полей\n");
			continue;
		}

		int rereg_reason = form_get_int_data(form, 9998, 0, 0);
		printf("rereg_reason = %d\n", rereg_reason);
		if (rereg_reason == 0) {
			message_box("Ошибка", "Не указана ни одна причина перерегистрации", dlg_yes, 0, al_center);
			form_focus(form, 9998);
			form_draw(form);
			continue;
		}

		if (fd_create_doc(RE_REGISTRATION, NULL, 0) != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("Ошибка", error, dlg_yes, 0, al_center);
			form_draw(form);
		} else
			break;
	}
	fa_set_group(FAPP_GROUP_MENU);
}

static int fa_get_int_field(form_t *form, uint16_t tag) {
	int ret = form_get_int_data(form, tag, 0, -1);
	if (ret == -1) {
		message_box("Ошибка", "Значение обязательного поля не указано", dlg_yes, 0, al_center);
		form_focus(form, tag);
		form_draw(form);
		return -1;
	}
	return ret;
}

void fa_cheque_corr() {
	const char *str_pay_type[] = { "Коррекция прихода", "Коррекция расхода" };
	const char *str_tax_mode[] = { "ОСН", "УСН ДОХОД", "УСН ДОХОД-РАСХОД", "ЕНВД", "ЕСХН" };
	const char *str_corr_type[] = { "Самостоятельно", "По предписанию" };

	BEGIN_FORM(cheque_corr_form, "Чек коррекции")
		FORM_ITEM_LISTBOX(1054, "Признак расчета:", str_pay_type, ASIZE(str_pay_type), -1)
		FORM_ITEM_LISTBOX(1055, "Система налогообложения:", str_tax_mode, ASIZE(str_tax_mode), -1)
		FORM_ITEM_LISTBOX(1173, "Тип коррекции:", str_corr_type, ASIZE(str_corr_type), -1)
		FORM_ITEM_EDIT_TEXT(1177, "Описание коррекции:", NULL, FORM_INPUT_TYPE_TEXT, 256)
		FORM_ITEM_EDIT_TEXT(1178, "Дата коррекции:", NULL, FORM_INPUT_TYPE_DATE, 10)
		FORM_ITEM_EDIT_TEXT(1179, "Номер предписания:", NULL, FORM_INPUT_TYPE_TEXT, 32)

		FORM_ITEM_EDIT_TEXT(1031, "Наличными:", "0", FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1081, "Безналичными:", "0", FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1215, "Предоплатой:", "0", FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1216, "Постоплатой:", "0", FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1217, "Встречным представлением:", "0", FORM_INPUT_TYPE_MONEY, 16)

		FORM_ITEM_EDIT_TEXT(1102, "НДС 20%:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1103, "НДС 10%:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1104, "НДС 0%:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1105, "БЕЗ НДС:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1106, "НДС 20/120:", NULL, FORM_INPUT_TYPE_MONEY, 16)
		FORM_ITEM_EDIT_TEXT(1107, "НДС 10/110:", NULL, FORM_INPUT_TYPE_MONEY, 16)

		FORM_ITEM_BUTTON(1, "Печать", NULL)
		FORM_ITEM_BUTTON(0, "Отмена", NULL)
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
			fa_tlv_add_string(form, 1177, 256, false, false) != 0 ||
			fa_tlv_add_unixtime(form, 1178, true) != 0 ||
			fa_tlv_add_string(form, 1179, 32, false, true) != 0 ||
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
			ffd_tlv_add_string(1021, data.cashier_post, 64, false);
		if (data.cashier_inn[0])
			ffd_tlv_add_string(1203, data.cashier_inn, 12, true);

		if (fd_create_doc(CHEQUE_CORR, NULL, 0) != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("Ошибка", error, dlg_yes, 0, al_center);
			form_draw(form);
		} else
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
	C *c;
	cheque_init();

	while ((c = cheque_execute())) {
		ffd_tlv_reset();
		cashier_data_t data = { "", "", "", "" };
		fa_load_cashier_data(&data);
		if (data.cashier_post[0])
			ffd_tlv_add_string(1021, data.cashier_post, 64, false);
		if (data.cashier_inn[0])
			ffd_tlv_add_string(1203, data.cashier_inn, 12, true);

		ffd_tlv_add_uint8(1054, c->t1054);
		ffd_tlv_add_uint8(1055, c->t1055);
		if (c->pe)
			ffd_tlv_add_string(1008, c->pe, 64, false);
		ffd_tlv_add_vln(1031, (uint64_t)c->sum.n);
		ffd_tlv_add_vln(1081, (uint64_t)c->sum.e);
		ffd_tlv_add_vln(1215, (uint64_t)c->sum.p);
		ffd_tlv_add_vln(1216, 0);
		ffd_tlv_add_vln(1217, 0);
		
		if (c->p != user_inn) {
			ffd_tlv_add_uint8(1057, 1 << 6);
			char phone[19+1];
			size_t size = get_phone(c->h, phone);
			
			ffd_tlv_add_string(1171, phone, size, false);
		}
		
		if (c->t1086 != NULL) {
			ffd_tlv_stlv_begin(1084, 320);
			ffd_tlv_add_string(1085, "ТЕРМИНАЛ", 64, false);
			ffd_tlv_add_string(1086, c->t1086, 256, false);
			ffd_tlv_stlv_end();
		}
		for (list_item_t *i1 = c->klist.head; i1; i1 = i1->next) {
			K *k = LIST_ITEM(i1, K);
			for (list_item_t *i2 = k->llist.head; i2; i2 = i2->next) {
				L *l = LIST_ITEM(i2, L);
				ffd_tlv_stlv_begin(1059, 1024);
				ffd_tlv_add_uint8(1214, l->r);
				char s1030[256];
				sprintf(s1030, "%s\n\rдокумент \xfc%lld", l->s, k->d);
				ffd_tlv_add_string(1030, s1030, 128, false);
				ffd_tlv_add_vln(1079, l->t);
				ffd_tlv_add_fvln(1023, 1, 0);
				if (l->n == 0)
					l->n = 6;
				ffd_tlv_add_vln(1199, l->n);
				if (l->n >= 1 && l->n <= 4) {
					ffd_tlv_add_vln(1198, l->c);
					ffd_tlv_add_vln(1200, l->c);
				}
				ffd_tlv_stlv_end();
			}
		}
		
		if (fd_create_doc(CHEQUE, NULL, 0) != 0) {
			const char *error;
			fd_get_last_error(&error);
			message_box("Ошибка", error, dlg_yes, 0, al_center);
			kbd_flush_queue();
			cheque_draw();
		} else {
			list_remove(&_ad->clist, c);
			AD_save();
			cheque_draw();
		}
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
				case cmd_cheque_fa:
					fa_cheque();
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
