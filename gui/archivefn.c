#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include "list.h"
#include "sysdefs.h"
#include "kbd.h"
#include "kkt/fd/tlv.h"
#include "kkt/fd/ad.h"
#include "kkt/fd/fd.h"
#include "kkt/kkt.h"
#include "paths.h"
#include "serialize.h"
#include "gui/gdi.h"
#include "gui/controls/controls.h"
#include "gui/controls/window.h"
#include "gui/dialog.h"
#include "gui/archivefn.h"
#include "gui/fa.h"

#define GAP 10
#define YGAP	2
#define TEXT_START		GAP
#define	CONTROLS_START	300 
#define CONTROLS_TOP	30
#define BUTTON_WIDTH	100
#define BUTTON_HEIGHT	30

static void text_free(char *text) {
	free(text);
}

static int op_kind;
static int res_out;
static uint32_t doc_no;
static list_t output = { NULL, NULL, 0, (list_item_delete_func_t)text_free };

static void archivefn_show_error(uint8_t status, const char *where) {
	const char *error;
	char error_text[512];

	fd_set_error(status, NULL, 0);
	fd_get_last_error(&error);
	snprintf(error_text, sizeof(error_text) - 1, "%s:\n%s", where, error_text);
	message_box("�訡��", error, dlg_yes, 0, al_center);
}

static void out_printf(const char *fmt, ...) {
	va_list args;
#define MAX_LEN	256
	char *text = malloc(MAX_LEN + 1);

	va_start(args, fmt);
	vsnprintf(text, MAX_LEN, fmt, args);
	va_end(args);
	
	list_add(&output, text);
}

static const char *get_doc_name(uint32_t type) {
	switch (type) {
		case REGISTRATION:
			return "���� � ॣ����樨";
		case RE_REGISTRATION:
			return "���� � ���ॣ����樨";
			break;
		case OPEN_SHIFT:
			return "���� �� ����⨨ ᬥ��";
		case CLOSE_SHIFT:
			return "���� � �����⨨ ᬥ��";
		case CALC_REPORT:
			return "���� � ⥪�饬 ���ﭨ� ��⮢";
		case CHEQUE:
			return "���";
		case CHEQUE_CORR:
			return "��� ���४樨";
		case BSO:
			return "���";
		case BSO_CORR:
			return "��� ���४樨";
		case CLOSE_FS:
			return "���� � �����⨨ ��";
		default:
			return NULL;
	}
}

struct kkt_report_hdr {
	uint8_t date_time[5];
	uint32_t doc_nr;
	uint32_t fiscal_sign;
} __attribute__((__packed__));

static void print_hdr(struct kkt_report_hdr *hdr) {
	out_printf(" ���/�६�: %.2d.%.2d.%.4d %.2d:%.2d", 
			hdr->date_time[2], hdr->date_time[1], (int)hdr->date_time[0] + 2000,
			hdr->date_time[3], hdr->date_time[4]);
	out_printf(" ����� ���-�: %u", hdr->doc_nr);
	out_printf(" ��: %u", hdr->fiscal_sign);
}

static const char *get_rereg_code(uint8_t code) {
	switch (code) {
		case 0:
			return "������ ��";
		case 1:
			return "������ ���";
		case 2:
			return "��������� ४����⮢";
		case 3:
			return "��������� ����஥� ���";
		default:
			return "�������⭠� ��稭�";
	}
}

static const char *get_pay_type(uint8_t type) {
	switch (type) {
		case 0:
			return "��室";
		case 1:
			return "������ ��室�";
		case 2:
			return "���室";
		case 3:
			return "����௠� ��室�";
		default:
			return "�������� ⨯";
	}
}

const char *str_kkt_modes_ex[] = { 
	"���", "�����. �.", "�������. �.",
	"������", "���", "��������",
	"��. ��.", "����. ��."
};

static void print_reg(struct kkt_reregister_report *p, bool reg) {
	char tax_systems[256];
	char kkt_modes[256];

	out_printf(" ���: %.12s", p->inn);
	out_printf(" ���: %.20s", p->reg_number);

	char *s = tax_systems;
	for (int i = 0, n = 0; i < str_tax_system_count; i++) {
		if (p->tax_system & (1 << i))
			s += sprintf(s, "%s%s", n++ > 0 ? "," : "", str_tax_systems[i]);
	}
	out_printf(" ���⥬� ���������������: %s", tax_systems);

	s = kkt_modes;
	for (int i = 0, n = 0; i < ASIZE(str_kkt_modes_ex); i++) {
		if (p->mode & (1 << i))
			s += sprintf(s, "%s%s", n++ > 0 ? "," : "", str_kkt_modes_ex[i]);
	}
	out_printf(" ������ ࠡ���: %s", kkt_modes);

	if (!reg)
		out_printf(" ��� ��稭� ���ॣ����樨", get_rereg_code(p->rereg_code));
}

static void print_shift(struct kkt_shift_report *p) {
	out_printf(" ����� ᬥ��: %d", p->shift_nr);
}

static void print_calc_report(struct kkt_calc_report *p) {
	out_printf(" ����।����� ��: %u", p->nr_uncommited);
	out_printf(" �� �� ��।��� �: %.2d.%.2d.%4d",
			p->first_uncommited_dt[2],
			p->first_uncommited_dt[1],
			(int)p->first_uncommited_dt[0] + 2000);
}

static void print_cheque(struct kkt_cheque_report *p) {
	uint64_t v = 
		(uint64_t)(p->sum[0] << 0) |
		((uint64_t)p->sum[1] << 8) |
		((uint64_t)p->sum[2] << 16) |
		((uint64_t)p->sum[3] << 24) |
		((uint64_t)p->sum[4] << 32);
	out_printf(" �ਧ��� ����: %s", get_pay_type(p->type));
	out_printf(" ����: %.1lld.%.2lld", v / 100, v % 100);
}

static void print_close_fs(struct kkt_close_fs *p) {
	out_printf(" ���: %.12s", p->inn);
	out_printf(" ���: %.20s", p->reg_number);
}

static void print_fdo_ack(struct kkt_fs_fdo_ack *fdo_ack) {
	char str_fs[18*2 + 1];
	char *s;

	out_printf("���⠭�� ���");

	out_printf(" ���/�६�: %.2d.%.2d.%.4d %.2d:%.2d", 
			fdo_ack->dt.date.day,
			fdo_ack->dt.date.month,
			(int)fdo_ack->dt.date.year + 2000,
			fdo_ack->dt.time.hour,
			fdo_ack->dt.time.minute);

	s = str_fs;
	for (int i = 0; i < 18; i++)
		s += sprintf(s, "%.2X", fdo_ack->fiscal_sign[i]);
	out_printf(" ��� ���: %s", str_fs);
}

static bool archivefn_get_archive_doc() {
	struct kkt_fs_find_doc_info fdi;
	struct kkt_fs_fdo_ack fdo_ack;
	uint8_t data[512];
	size_t data_len;
	uint8_t status;

   	if ((status = kkt_find_fs_doc(doc_no, res_out != 0, &fdi, data, &data_len)) != 0) {
		archivefn_show_error(status, "�訡�� �� ����祭�� ���㬥�� �� ��娢� ��");
		return false;
	}

	if (fdi.fdo_ack) {
		if ((status = kkt_find_fdo_ack(doc_no, false, &fdo_ack)) != 0) {
			archivefn_show_error(status, "�訡�� �� ����祭�� ���⠭樨 ��� �� ��娢� ��");
			return false;
		}
	}

/*	fdi.doc_type = REGISTRATION;
	fdi.fdo_ack = true;

	struct kkt_reregister_report *p = (struct kkt_reregister_report *)data;
	p->date_time[0] = 19;
	p->date_time[1] = 1;
	p->date_time[2] = 27;
	p->date_time[3] = 17;
	p->date_time[4] = 39;
	p->doc_nr = 25;
	p->fiscal_sign = 2342453634;
	memcpy(p->inn, "1234567890  ", 12);
	memcpy(p->reg_number, "1234567890123456    ", 20);
	p->tax_system = 4;
	p->mode = 1 + 8 + 32;
	p->rereg_code = 1;

	fdo_ack.dt.date.year = 19;
	fdo_ack.dt.date.month = 1;
	fdo_ack.dt.date.day = 27;
	fdo_ack.dt.time.hour = 17;
	fdo_ack.dt.time.minute = 43;*/

	list_clear(&output);

	const char *doc_name = get_doc_name(fdi.doc_type);
	out_printf("%s", doc_name);
	print_hdr((struct kkt_report_hdr *)data);
	
	switch (fdi.doc_type) {
		case REGISTRATION:
		case RE_REGISTRATION:
			print_reg((struct kkt_reregister_report *)data, fdi.doc_type == REGISTRATION);
			break;
		case OPEN_SHIFT:
		case CLOSE_SHIFT:
			print_shift((struct kkt_shift_report *)data);
			break;
		case CALC_REPORT:
			print_calc_report((struct kkt_calc_report *)data);
			break;
		case CHEQUE:
		case CHEQUE_CORR:
		case BSO:
		case BSO_CORR:
			print_cheque((struct kkt_cheque_report *)data);
			break;
		case CLOSE_FS:
			print_close_fs((struct kkt_close_fs *)data);
			break;
	}

	if (fdi.fdo_ack)
		print_fdo_ack(&fdo_ack);

	return true;
}

static bool archivefn_get_doc() {
	uint8_t status;
	uint16_t doc_type;
	size_t tlv_size;
	uint8_t *tlv;
	uint8_t *p;

	if ((status = kkt_get_doc_stlv(doc_no, &doc_type, &tlv_size)) != 0) {
		archivefn_show_error(status, "�訡�� �� ����祭�� ���ଠ樨 � ���㬥��");
		return false;
	}

	tlv = (uint8_t *)malloc(tlv_size);
	if (!tlv) {
		message_box("�訡��", "�訡�� �� �뤥����� ����", dlg_yes, 0, al_center);
		return false;
	}
	p = tlv;

	while (true) {
		size_t len;
		if ((status = kkt_read_doc_tlv(p, &len)) == 0)
			p += len;
		else { 
			if (status != 0x8) 
				archivefn_show_error(status, "�訡�� �� �⥭�� TLV �� ��");
			break;
		}
	}

	free(tlv);

	return !status || status == 8;
}

static bool archivefn_get_data() {
	if (op_kind == 0)
		return archivefn_get_archive_doc();
	return archivefn_get_doc();
}

static void button_action(control_t *c, int cmd) {
	window_set_dialog_result(((window_t *)c->parent.parent), cmd);
}

static void listbox_get_item_text(void *obj, char *text, size_t text_size) {
	snprintf(text, text_size, "%s", (char *)obj);
}

int archivefn_execute() {
	window_t *win = window_create(NULL, 
			"��ᬮ��/����� ���㬥�⮢ �� �� (��娢� ��) (Esc - ��室)", NULL);
	GCPtr screen = window_get_gc(win);

	int x = CONTROLS_START;
	int y = CONTROLS_TOP;
	int w = DISCX - x - GAP;
	int th = GetTextHeight(screen);
	int h = th + 8;

	const char *str_op_kind[] = {
		"���㬥�� �� ��娢� ��",	
		"���㬥�� �� ��",
	};
	const char *str_res_out[] = {
		"�� �࠭",
		"�� �࠭ � �� �����"
	};
	char str_doc_no[16];

	if (doc_no != 0)
		sprintf(str_doc_no, "%d", doc_no);

	window_add_label(win, TEXT_START, y, align_left, "��� ����樨:");
	window_add_control(win, 
			simple_combobox_create(9999, screen, x, y, w, h, 
				str_op_kind, ASIZE(str_op_kind), op_kind));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y, align_left, "�뢮� १����:");
	window_add_control(win, 
			simple_combobox_create(9998, screen, x, y, w, h, 
				str_res_out, ASIZE(str_res_out), res_out));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y, align_left, "����� ���㬥��:");
	window_add_control(win, 
			edit_create(1040, screen, x, y, w, h, doc_no == 0 ? NULL : str_doc_no,
				EDIT_INPUT_TYPE_TEXT, 10));
	y += h + YGAP + th + 12;

	window_add_label(win, TEXT_START, y - th - 4, align_left, "����� ���㬥��:");

	window_add_control(win,
			listbox_create(9997, screen, GAP, y, DISCX - GAP*2, DISCY - y - BUTTON_HEIGHT - GAP*2,
				&output, listbox_get_item_text, 0));

	window_add_control(win, 
			button_create(1, screen, DISCX - (BUTTON_WIDTH + GAP)*2, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 1, "�믮�����", button_action));
	window_add_control(win, 
			button_create(0, screen, DISCX - (BUTTON_WIDTH + GAP)*1, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 0, "�������", button_action));

	int result;
	while (true) {
		result = window_show_dialog(win);
		data_t data;

		op_kind = window_get_int_data(win, 9999, 0, 0);
		res_out = window_get_int_data(win, 9998, 0, 0);

		if (result == 0)
			break;

		window_get_data(win, 1040, 0, &data);

		if (data.size == 0) {
			window_show_error(win, 1040, "���� \"����� ���㬥��\" �� ���������");
			continue;
		}
		doc_no = atoi(data.data);
		if (doc_no == 0) {
			window_show_error(win, 1040, "������ ���祭�� ���� \"����� ���㬥��\" �������⨬�");
			continue;
		}

		if (!archivefn_get_data())
			continue;
	}

	window_destroy(win);

	return result;
}
