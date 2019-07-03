#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include "list.h"
#include "sysdefs.h"
#include "kbd.h"
#include "kkt/fd/tlv.h"
#include "kkt/fd/ad.h"
#include "kkt/kkt.h"
#include "paths.h"
#include "serialize.h"
#include "gui/gdi.h"
#include "gui/controls/controls.h"
#include "gui/controls/window.h"
#include "gui/dialog.h"
#include "gui/newcheque.h"
#include "gui/fa.h"
#include "gui/references/agent.h"
#include "gui/references/article.h"

#define GAP 10
#define YGAP	2
#define TEXT_START		GAP
#define	CONTROLS_START	300 
#define CONTROLS_TOP	30
#define BUTTON_WIDTH	100
#define BUTTON_HEIGHT	30

typedef struct {
	article_t *article;
	ffd_fvln_t count;
	uint64_t price_per_unit;
	uint64_t sum;
} cheque_article_t;

typedef struct {
	int tax_system;
	int pay_type;
	int pay_kind;
	char* phone_or_email;
	char* add_info;
	uint64_t sum;
	list_t articles;
} newcheque_t;

static cheque_article_t *cheque_article_new() {
	return (cheque_article_t *)malloc(sizeof(cheque_article_t));
}

static void cheque_article_free(cheque_article_t *ca) {
	free(ca);
}

static newcheque_t newcheque = {
	.pay_type = 1,
	.tax_system = 0,
	.pay_kind = 0,
	.phone_or_email = NULL,
	.add_info = NULL,
	.sum = 0,
	.articles = { NULL, NULL, 0, (list_item_delete_func_t)cheque_article_free },
};

extern bool check_phone_or_email(const char *pe);


/*static double cheque_article_sum(cheque_article_t *ca) {
	double count = ffd_fvln_to_double(&ca->count);
	double price_per_unit = (ca->article ? (double)ca->article->price_per_unit : 0) / 100.0;
	return (count * price_per_unit);
}*/

static uint64_t cheque_article_vln_sum(cheque_article_t *ca) {
//	printf("price_per_unit: %lld, count->value = %lld, count->dot = %d\n",
//			ca->article->price_per_unit, ca->count.value, ca->count.dot);

	uint64_t sum = ca->price_per_unit * ca->count.value;
	uint64_t rem = 0;
	for (int i = 0; i < ca->count.dot; i++) {
		rem = sum % 10;
		sum /= 10;
	}
	if (rem >= 5)
		sum++;
//	printf("sum: %lld\n", sum);
	return sum;
}

static int cheque_article_save(int fd, cheque_article_t *a) {
	int i = 0;
	for (list_item_t *li = articles.head; li; li = li->next, i++) {
		if (li->obj == a->article)
			break;
	}

	if (SAVE_INT(fd, i) ||
			SAVE_INT(fd, a->count.value) ||
			SAVE_INT(fd, a->count.dot) ||
			SAVE_INT(fd, a->price_per_unit))
		return -1;
	return 0;
}

static cheque_article_t * cheque_article_load(int fd) {
	cheque_article_t *a = cheque_article_new();
	int article_index = -1;

	if (LOAD_INT(fd, article_index) ||
			LOAD_INT(fd, a->count.value) ||
			LOAD_INT(fd, a->count.dot) ||
			LOAD_INT(fd, a->price_per_unit)) {
		free(a);
		return NULL;
	}

	a->article = NULL;
	int i = 0;
	for (list_item_t *li = articles.head; li; li = li->next, i++) {
		if (i == article_index) {
			a->article = LIST_ITEM(li, article_t);
			break;
		}
	}

	if (a->article == NULL) {
		free(a);
		return NULL;
	}
	a->sum = cheque_article_vln_sum(a);
	newcheque.sum += a->sum;

	return a;
}

#define FILE_NAME	"/home/sterm/newcheque.dat"
static bool newcheque_save() {
	bool ret = false;
	int fd = s_open(FILE_NAME, true);
	if (fd != -1) {
		ret = SAVE_INT(fd, newcheque.pay_type) == 0 &&
			SAVE_INT(fd, newcheque.pay_kind) == 0 &&
			save_string(fd, newcheque.add_info) == 0 &&
			save_list(fd, &newcheque.articles, (list_item_func_t)cheque_article_save) == 0;

		s_close(fd);
	}

	return ret;
}

bool newcheque_load() {
	bool ret = false;
	int fd = s_open(FILE_NAME, false);
	if (fd != -1) {
		ret = LOAD_INT(fd, newcheque.pay_type) == 0 &&
			LOAD_INT(fd, newcheque.pay_kind) == 0 &&
			load_string(fd, &newcheque.add_info) == 0 &&
			load_list(fd, &newcheque.articles, (load_item_func_t)cheque_article_load) == 0;

		s_close(fd);
	}

	return ret;
}


static void button_action(control_t *c, int cmd) {
	window_set_dialog_result(((window_t *)c->parent.parent), cmd);
}

static int article_get_text(void *obj, int index, char *text, size_t text_size) {
	article_t *a = (article_t *)obj;

	switch (index) {
		case 0:
			snprintf(text, text_size, "%d", a->n);
			break;
		case 1:
			snprintf(text, text_size, "%s", a->name);
			break;
		case 2:
			snprintf(text, text_size, "%.1lld.%.2lld",
					a->price_per_unit / 100, a->price_per_unit % 100);
			break;
		case 3:
			for (list_item_t *li = agents.head; li; li = li->next) { 
				agent_t *agent = LIST_ITEM(li, agent_t);
				if (a->pay_agent == agent->n) {
					snprintf(text, text_size, "%s", agent->name);
					return 0;
				}
			}
			text[0] = 0;
			break;
		default:
			return -1;
	}
	return 0;
}

static void newcheque_update_sum(window_t *w, bool redraw) {
	char text[256];
	sprintf(text, "�����: %.1lld.%.2lld", newcheque.sum / 100, newcheque.sum % 100);
	window_set_label_text(w, 1, text, redraw);
}

static void article_selected_changed(control_t *c, int index, void *item) {
	window_t *win = (window_t *)c->parent.parent;
	char text[64] = "";

	if (win) {
		control_t *c = window_get_control(win, 1079);

		if (item) {
			article_t *a = (article_t *)item;
			snprintf(text, sizeof(text), "%.1lld.%.2lld",
					a->price_per_unit / 100, a->price_per_unit % 100);
			c->enabled = a->price_per_unit == 0;
		}

		control_set_data(c, 0, text, strlen(text));
	}
}

static void article_new(window_t *parent) {
	bool added = false;
	window_t *win = window_create(NULL, "�롥�� ⮢��/ࠡ���/����", NULL);
	GCPtr screen = window_get_gc(win);
	listview_column_t columns[] = {
		{ "\xfc", 30 },
		{ "������������", 320 },
		{ "���� �� ��.", 120 },
		{ "���⠢騪", 158 },
	};

	int x = CONTROLS_START;
	int y = CONTROLS_TOP;
	int w = DISCX - x - GAP;
	int th = GetTextHeight(screen);
	int h = DISCY - 200;

	window_add_label(win, TEXT_START, y, align_left, "���᮪ ⮢�஢/ࠡ��/���:");
	y += th + 2;

	window_add_control(win, 
			listview_create(1059, screen, TEXT_START, y, DISCX - GAP * 2, h, columns, ASIZE(columns),
				&articles, (listview_get_item_text_func_t)article_get_text, 
				article_selected_changed, 0));
	y += h + GAP;

	window_add_label(win, TEXT_START, y + 4, align_left, "���� �� ��.:");
	window_add_control(win,
			edit_create(1079, screen, TEXT_START + 150, y, w, th + 8, NULL, EDIT_INPUT_TYPE_MONEY, 14));
	y += 40;

	window_add_label(win, TEXT_START, y + 4, align_left, "���-��:");
	window_add_control(win,
			edit_create(1023, screen, TEXT_START + 150, y, w, th + 8, NULL, EDIT_INPUT_TYPE_DOUBLE, 16));


	x = (DISCX - (BUTTON_WIDTH + GAP)*2 - GAP) / 2;
	window_add_control(win, 
			button_create(1, screen, x, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 1, "��������", button_action));
	window_add_control(win, 
			button_create(0, screen, x + BUTTON_WIDTH + GAP,
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 0, "�⬥��", button_action));

	window_set_data(win, 1059, 1, NULL, 0);

	int result;
	while ((result = window_show_dialog(win, -1)) == 1) {
		article_t *a = window_get_ptr_data(win, 1059, 1);
		if (a == NULL) {
			window_show_error(win, 1059, "�� ��࠭ ⮢��/ࠡ��/��㣠");
			continue;
		} 
		data_t data;
		window_get_data(win, 1023, 0, &data);
		if (data.size == 0) {
			window_show_error(win, 1023, "�� 㪠���� ���-�� �।��� ����");
			continue;
		}

		ffd_fvln_t fvln;
		if (!ffd_string_to_fvln(data.data, data.size, &fvln) || fvln.value == 0) {
			window_show_error(win, 1059, "���-�� �।��� ���� 㪠���� ����୮");
			continue;
		}

		window_get_data(win, 1079, 0, &data);
		if (data.size == 0) {
			window_show_error(win, 1079, "�� 㪠���� 業� �� ��. �।��� ����");
			continue;
		}

		uint64_t vln;
		if (!ffd_string_to_vln(data.data, data.size, &vln)) {
			window_show_error(win, 1079, "���� �� ��. �।��� ���� 㪠���� ����୮");
			continue;
		}

		cheque_article_t *ca;
		if (newcheque.articles.head) {
			ca = LIST_ITEM(newcheque.articles.head, cheque_article_t);
			if (ca->article->pay_agent != a->pay_agent) {
				window_show_error(win, 1059, 
					"����� ��������� � ���� 祪 ⮢��� (��㣨) ࠧ��� ���⠢騪��");
				continue;
			}
		}

		ca = cheque_article_new();
		ca->article = a;
		ca->count = fvln;
		ca->price_per_unit = vln;
		ca->sum = cheque_article_vln_sum(ca);

		newcheque.sum += ca->sum;

		list_add(&newcheque.articles, ca);

		window_set_data(parent, 9997, 0, (void *)(newcheque.articles.count - 1), 0);

		newcheque_update_sum(parent, false);
		added = true;

		break;
	}

	window_destroy(win);
	window_draw(parent);
	if (added)
		window_set_focus(parent, 9997);
}

static void article_delete(window_t *parent) {
	int index = window_get_int_data(parent, 9997, 0, -1);
	list_item_t *li = list_item_at(&newcheque.articles, index);

	if (li) {
		cheque_article_t *ca = LIST_ITEM(li, cheque_article_t);
		uint64_t sum = ca->sum;

		list_remove(&newcheque.articles, ca);

		newcheque.sum -= sum;
		newcheque_update_sum(parent, true);
		window_redraw_control(parent, 9997);
	}
}

static void listbox_get_item_text(void *obj, char *text, size_t text_size) {
	cheque_article_t *ca = (cheque_article_t *)obj;

	double count = ffd_fvln_to_double(&ca->count);
	//double price_per_unit = (ca->article ? (double)ca->article->price_per_unit : 0) / 100.0;
	//double sum = (count * price_per_unit);
	snprintf(text, text_size, "%.1lld.%.2lld*%.3f=%.1lld.%.2lld %s",
			ca->price_per_unit / 100, ca->article->price_per_unit % 100,
			count, ca->sum / 100, ca->sum % 100, 
			ca->article ? ca->article->name :
			"<�����/ࠡ��/��㣠 �� ������(�) � �ࠢ�筨��>");
}

bool newcheque_process(window_t *w, const struct kbd_event *e) {
	control_t *c;
	if (!e->pressed)
		return true;

	switch (e->key) {
		case KEY_NUMPLUS:
		case KEY_PLUS:
			c = window_get_focus(w);
			if (c && c->id == 9997)
				article_new(w);
			break;
		case KEY_MINUS:
		case KEY_NUMMINUS:
			c = window_get_focus(w);
			if (c && c->id == 9997)
				article_delete(w);
			break;
	}
	return true;
}


typedef struct {
	int agent_id;
} article_group_params_t;

int remove_articles(article_group_params_t *p, cheque_article_t *a) {
	if (a->article->pay_agent == p->agent_id)
		return 0;
	return -1;
}

static bool newcheque_distribute_sum(window_t *w, uint64_t sum[5]) {
	window_t *win = window_create(NULL, "���।���� �㬬� �� ����� ������", NULL);
	GCPtr screen = window_get_gc(win);

	int x = CONTROLS_START;
	int y = CONTROLS_TOP;
	int width = DISCX - x - GAP - 100;
	int th = GetTextHeight(screen);
	int height = th + 8;
	char text[256];
	uint64_t total_sum = newcheque.sum;

	/*for (list_item_t *li = newcheque.articles.head; li; li = li->next) {
		cheque_article_t *ca = LIST_ITEM(li, cheque_article_t);
		total_sum += ca->sum;
	}*/
	sprintf(text, "%.1llu.%.2llu ���.", total_sum / 100, total_sum % 100);

	window_add_label(win, TEXT_START, y, align_left, "���� �㬬� 祪�:");
	window_add_label(win, TEXT_START + 350, y, align_left, text);
	y += height + YGAP*2;

	window_add_label(win, TEXT_START, y, align_left, "�����묨:");
	window_add_control(win,
		edit_create(1031, screen, TEXT_START + 350, y, width, th + 8, NULL, EDIT_INPUT_TYPE_MONEY, 16));
	y += height + YGAP;

	window_add_label(win, TEXT_START, y, align_left, "��������묨:");
	window_add_control(win,
		edit_create(1081, screen, TEXT_START + 350, y, width, th + 8, NULL, EDIT_INPUT_TYPE_MONEY, 16));
	y += height + YGAP;

	window_add_label(win, TEXT_START, y, align_left, "� ���� ࠭�� ���ᥭ��� �।��:");
	window_add_control(win,
		edit_create(1215, screen, TEXT_START + 350, y, width, th + 8, NULL, EDIT_INPUT_TYPE_MONEY, 16));
	y += height + YGAP;

	window_add_label(win, TEXT_START, y, align_left, "���⮯��⮩ (�।��):");
	window_add_control(win,
		edit_create(1216, screen, TEXT_START + 350, y, width, th + 8, NULL, EDIT_INPUT_TYPE_MONEY, 16));
	y += height + YGAP;

	window_add_label(win, TEXT_START, y, align_left, "������ �।��⠢������:");
	window_add_control(win,
		edit_create(1217, screen, TEXT_START + 350, y, width, th + 8, NULL, EDIT_INPUT_TYPE_MONEY, 16));
	y += height + YGAP;

	x = (DISCX - (BUTTON_WIDTH + GAP)*2 - GAP) / 2;
	window_add_control(win,
			button_create(1, screen, x,
				DISCY - BUTTON_HEIGHT - GAP,
				BUTTON_WIDTH, BUTTON_HEIGHT, 1, "�����", button_action));
	window_add_control(win, 
			button_create(0, screen, x + BUTTON_WIDTH + GAP,
				DISCY - BUTTON_HEIGHT - GAP,
				BUTTON_WIDTH, BUTTON_HEIGHT, 0, "�⬥��", button_action));

	bool ret = false;
	int result;
	while ((result = window_show_dialog(win, -1)) == 1) {
		uint16_t tags[] = { 1031, 1081, 1215, 1216, 1217 };
		uint64_t all_sum = 0;

		for (int i = 0; i < ASIZE(tags); i++) {
			data_t data;
			char *endp;
			window_get_data(win, tags[i], 1, &data);

			if (data.size > 0) {
				double v = strtod((const char *)data.data, &endp);
				if (*endp != 0) {
					window_show_error(win, tags[i], "�訡�� �� ����� �㬬�");
					continue;
				} else {
					sum[i] = (uint64_t)((v + 0.009) * 100.0);
					//printf("%.4d: %llu\n", tags[i], sum[i]);
					all_sum += sum[i];
				}
			}
		}
		
		//printf("all_sum: %llu, total_sum: %llu\n", all_sum, total_sum);
		
		if (all_sum == total_sum) {
			ret = true;
			break;
		}
		window_show_error(win, tags[0], "�訡��: �㬬� ��� ����� ������ ���� ࠢ�� ��饩 �㬬� 祪�");
	}
	window_draw(w);
	kbd_flush_queue();
	return ret;
}

static void newcheque_show_op(window_t *w, const char *title) {
	GCPtr screen = window_get_gc(w);
	SetGCBounds(screen, 0, 0, DISCX, DISCY);

	SetTextColor(screen, clBlack);
	fill_rect(screen, 100, 240, DISCX - 100*2, DISCY - 240*2, 2, clRopnetDarkBrown, clRopnetBrown);
	DrawText(screen, 100, 240, DISCX - 100*2, DISCY - 240*2, title, DT_CENTER | DT_VCENTER);
}

extern uint8_t reg_tax_systems;
static const char * s_tax_systems[6];
static uint8_t b_tax_systems[6];
static int s_tax_system_count;

bool newcheque_print(window_t *w) {
	data_t cashier;
	data_t post;
	data_t inn;
	uint64_t dsum[5] = { 0, 0, 0, 0, 0};

	window_get_data(w, 1021, 1, &cashier);
	window_get_data(w, 9999, 1, &post);
	window_get_data(w, 1203, 1, &inn);

	if (newcheque.phone_or_email && newcheque.phone_or_email[0] && !check_phone_or_email(newcheque.phone_or_email)) {
		window_show_error(w, 1008, "����� ⥫. ��� e-mail ����� �������⨬� �ଠ�.\n"
			"�ਬ�� �����: +71111111111 ��� name@mail.ru");
		return false;
	}

	if (cashier.size == 0) {
		window_show_error(w, 1021, "��易⥫쭮� ���� �� ���������");
		return false;
	}

	if (!cashier_set(cashier.data, post.data, inn.data)) {
		window_show_error(w, 1021, "�訡�� ����� � 䠩� ������ � �����");
		return false;
	}
	
	
	if (newcheque.pay_type < 1 || newcheque.pay_type > 4) {
		window_show_error(w, 1054, "�롥�� �ਧ��� ����");
		return false;
	}

	if (newcheque.pay_kind == 4 && !newcheque_distribute_sum(w, dsum))
		return false;
		
	while (newcheque.articles.count > 0) {
		cheque_article_t *ca = LIST_ITEM(newcheque.articles.head, cheque_article_t);
		article_group_params_t p = { ca->article->pay_agent };
		uint64_t sum = 0;
		agent_t *agent = NULL;

		if (p.agent_id) {
			for (list_item_t *li = agents.head; li; li = li->next) {
				agent_t *a = LIST_ITEM(li, agent_t);
				if (p.agent_id == a->n) {
					agent = a;
					break;
				}
			}
		} else
			agent = NULL;

		printf("cashier: %s\n", cashier_get_cashier());

		ffd_tlv_reset();
		ffd_tlv_add_string(1021, cashier_get_cashier());
		const char *cashier_inn = cashier_get_inn();
		if (cashier_inn[0])
			ffd_tlv_add_fixed_string(1203, cashier_inn, 12);
		ffd_tlv_add_uint8(1054, newcheque.pay_type);

		printf("p.tax_system = %d, p.agent = %d\n", newcheque.tax_system, p.agent_id);

		ffd_tlv_add_uint8(1055, b_tax_systems[newcheque.tax_system]);
		if (newcheque.phone_or_email && newcheque.phone_or_email[0])
			ffd_tlv_add_string(1008, newcheque.phone_or_email);

		if (_ad->t1086 != NULL) {
			ffd_tlv_stlv_begin(1084, 320);
			ffd_tlv_add_string(1085, "��������");
			ffd_tlv_add_string(1086, _ad->t1086);
			ffd_tlv_stlv_end();
		}

		if (newcheque.add_info && newcheque.add_info[0])
			ffd_tlv_add_string(1192, newcheque.add_info);

		for (list_item_t *li = newcheque.articles.head; li; li = li->next) {
			cheque_article_t *ca = LIST_ITEM(li, cheque_article_t);
			if (ca->article->pay_agent == p.agent_id /*&& ca->article->tax_system == p.tax_system*/) {
				printf("add new 1059\n");
				ffd_tlv_stlv_begin(1059, 1024);
				ffd_tlv_add_uint8(1214, ca->article->pay_method);
				ffd_tlv_add_string(1030, ca->article->name);
				ffd_tlv_add_vln(1079, ca->price_per_unit);
				ffd_tlv_add_fvln(1023, ca->count.value, ca->count.dot);
				ffd_tlv_add_uint8(1199, ca->article->vat_rate);
				if (agent != NULL)
					ffd_tlv_add_fixed_string(1226, agent->inn, 12);
				ffd_tlv_stlv_end();

				printf("sum: %lld\n", ca->sum);

				sum += ca->sum;
			}
		}

		if (agent != NULL) {
			ffd_tlv_add_uint8(1057, 1 << agent->pay_agent);
			switch (agent->pay_agent) {
				case 0:
				case 1:
					ffd_tlv_add_string(1075, agent->transfer_operator_phone);
					ffd_tlv_add_string(1044, agent->pay_agent_operation);
					ffd_tlv_add_string(1073, agent->pay_agent_phone);
					ffd_tlv_add_string(1026, agent->money_transfer_operator_name);
					ffd_tlv_add_string(1005, agent->money_transfer_operator_address);
					ffd_tlv_add_fixed_string(1016, agent->money_transfer_operator_inn, 12);
					ffd_tlv_add_string(1171, agent->supplier_phone);
					break;
				case 2:
				case 3:
					ffd_tlv_add_string(1073, agent->pay_agent_phone);
					ffd_tlv_add_string(1174, agent->payment_processor_phone);
					ffd_tlv_add_string(1171, agent->supplier_phone);
					break;
				case 4:
				case 5:
				case 6:
					ffd_tlv_add_string(1171, agent->supplier_phone);
					break;
			}
		}

		switch (newcheque.pay_kind) {
		case 0:
			dsum[0] = sum;
			break;
		case 1:
			dsum[1] = sum;
			break;
		case 2:
			dsum[2] = sum;
			break;
		case 3:
			dsum[4] = sum;
			break;
		}

		ffd_tlv_add_vln(1031, dsum[0]);
		ffd_tlv_add_vln(1081, dsum[1]);
		ffd_tlv_add_vln(1215, dsum[2]);
		ffd_tlv_add_vln(1216, dsum[3]);
		ffd_tlv_add_vln(1217, dsum[4]);
		
/*		printf("dsum[0] = %llu\n", dsum[0]);
		printf("dsum[1] = %llu\n", dsum[1]);
		printf("dsum[2] = %llu\n", dsum[2]);
		printf("dsum[3] = %llu\n", dsum[3]);
		printf("dsum[4] = %llu\n", dsum[4]);*/

		cashier_save();

		newcheque_show_op(w, "���� ����� 祪�...");

		if (!fa_create_doc(CHEQUE, NULL, 0, NULL, NULL))
			return false;

		list_remove_if(&newcheque.articles, &p, (list_item_func_t)remove_articles);

		newcheque.sum -= sum;
		newcheque_update_sum(w, false);

		newcheque_save();
		window_draw(w);
		kbd_flush_queue();
	}

	return true;
}

int newcheque_execute() {
	int focus_id = 9997;

	window_t *win = window_create(NULL, "���(�) (Esc - ��室)", newcheque_process);
	GCPtr screen = window_get_gc(win);
	
	s_tax_system_count = 0;
	for (int i = 0; i < str_tax_system_count; i++) {
		uint8_t b = 1 << i;
		if (reg_tax_systems & b) {
			s_tax_systems[s_tax_system_count] = str_tax_systems[i];
			b_tax_systems[s_tax_system_count] = b;
			s_tax_system_count++;
		}
	}

	int x = CONTROLS_START;
	int y = CONTROLS_TOP;
	int w = DISCX - x - GAP;
	int th = GetTextHeight(screen);
	int h = th + 8;
#define DY 2

	const char *pay_type[] = {
		"��室",
		"������ ��室�",
		"���室",
		"������ ��室�"
	};
	
	const char *pay_kind[] = {
		"������",
		"���������",
		"� ���� ࠭�� ���ᥭ��� �।��",
		"������ �।��⠢������",
		"���।������ �� ����� ������"
	};

	if (newcheque.tax_system >= s_tax_system_count)
		newcheque.tax_system = 0;

	window_add_label(win, TEXT_START, y, align_left, "�ਧ��� ����:");
	window_add_control(win, 
			simple_combobox_create(1054, screen, x, y, w, h, pay_type, ASIZE(pay_type), 
				newcheque.pay_type - 1));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y, align_left, "���:");
	window_add_control(win, 
				simple_combobox_create(1055, screen, x, y, w, h, s_tax_systems, s_tax_system_count, 
				newcheque.tax_system));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y, align_left, "��� ������:");
	window_add_control(win, 
			simple_combobox_create(9998, screen, x, y, w, h, pay_kind, ASIZE(pay_kind), 
				newcheque.pay_kind));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y + DY, align_left, "�����:");
	window_add_control(win, 
			edit_create(1021, screen, x, y, w, h, cashier_get_name(), EDIT_INPUT_TYPE_TEXT, 64));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y + DY, align_left, "���������:");
	window_add_control(win,
			edit_create(9999, screen, x, y, w, h, cashier_get_post(), EDIT_INPUT_TYPE_TEXT, 64));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y + DY, align_left, "��� �����:");
	window_add_control(win,
			edit_create(1203, screen, x, y, w, h, cashier_get_inn(), EDIT_INPUT_TYPE_TEXT, 12));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y + DY, align_left, "���. ��� e-mail ���㯠⥫�:");
	window_add_control(win,
			edit_create(1008, screen, x, y, w, h, newcheque.phone_or_email,
			   	EDIT_INPUT_TYPE_TEXT, 64));
	y += h + YGAP;

	window_add_label(win, TEXT_START, y + DY, align_left, "���. ४����� 祪�:");
	window_add_control(win,
			edit_create(1192, screen, x, y, w, h, newcheque.add_info,
			   	EDIT_INPUT_TYPE_TEXT, 16));
	y += h + YGAP + th + 12;

	window_add_label(win, TEXT_START, y - th - 4, align_left,
			"�।���� ���� (\"+\" ��������, \"-\" 㤠����):");


	char text[256];
	sprintf(text, "�����: %.1lld.%.2lld", newcheque.sum / 100, newcheque.sum % 100);
	window_add_label_with_id(win, 1, DISCX - GAP, y - th - 4, align_right, text);

	window_add_control(win,
			listbox_create(9997, screen, GAP, y, DISCX - GAP*2, DISCY - y - BUTTON_HEIGHT - GAP*2,
				&newcheque.articles, listbox_get_item_text, 0));

	window_add_control(win, 
			button_create(1, screen, DISCX - (BUTTON_WIDTH + GAP)*2, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 1, "�����", button_action));
	window_add_control(win, 
			button_create(0, screen, DISCX - (BUTTON_WIDTH + GAP)*1, 
				DISCY - BUTTON_HEIGHT - GAP, 
				BUTTON_WIDTH, BUTTON_HEIGHT, 0, "�������", button_action));

/*	if (newcheque.articles.count < 25) {
		for (int i = newcheque.articles.count; i < 25; i++) {
			cheque_article_t *ca = cheque_article_new();
			ca->article = LIST_ITEM(articles.head, article_t);
			ca->count.value = i + 1;
			ca->count.dot = 0;

			newcheque.sum += cheque_article_sum(ca);

			list_add(&newcheque.articles, ca);
		}
	}*/

	int result;
	while (true) {
		result = window_show_dialog(win, focus_id);
		focus_id = -1;

		data_t data;
		window_get_data(win, 1008, 1, &data);

		if (newcheque.phone_or_email)
			free(newcheque.phone_or_email);
		newcheque.phone_or_email = strdup(data.data);

		window_get_data(win, 1192, 0, &data);
		if (newcheque.add_info)
			free(newcheque.add_info);
		newcheque.add_info = strdup(data.data);

		newcheque.pay_type = window_get_int_data(win, 1054, 0, 0) + 1;
		newcheque.pay_kind = window_get_int_data(win, 9998, 0, 0);
		newcheque.tax_system = window_get_int_data(win, 1055, 0, 0);

		newcheque_save();

		if (result == 0)
			break;

		if (newcheque_print(win))
			break;
	}

	window_destroy(win);

	return result;
}


void on_newcheque_article_removed(article_t *a) {
	for (list_item_t *li = newcheque.articles.head; li; li = li->next) {
		cheque_article_t *ca = LIST_ITEM(li, cheque_article_t);
		if (ca->article == a) {
			newcheque.sum -= ca->sum;
			list_remove(&newcheque.articles, ca);
			break;
		}
	}
}
