/*
 * Преобразование записей циклической контрольной ленты (ЦКЛ)
 * в текстовый формат (Translate eXpress Log). (c) gsr 2009.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "log/express.h"
#include "prn/express.h"
#include "genfunc.h"
#include "paths.h"
#include "sterm.h"

/* Является ли заданный символ десятичной цифрой */
static bool is_digit(uint8_t b)
{
	return (b >= 0x30) && (b <= 0x39);
}

/* Вывод заголовка текущей записи */
static void write_header(void)
{
	printf("---#%u/%u---%.2u.%.2u.%.2u %.2u:%.2u:%.2u---"
			"╥╠ %.2hhX:%.2hhX--%c\r\n",
		xlog_rec_hdr.number + 1, xlog_rec_hdr.n_para + 1,
		xlog_rec_hdr.dt.day + 1, xlog_rec_hdr.dt.mon + 1, xlog_rec_hdr.dt.year,
		xlog_rec_hdr.dt.hour, xlog_rec_hdr.dt.min, xlog_rec_hdr.dt.sec,
		xlog_rec_hdr.addr.gaddr, xlog_rec_hdr.addr.iaddr,
		xlog_rec_hdr.printed ? '+' : '-');
}

/* Вывод содержимого текущей записи */
static void write_data(void)
{
	enum {
		st_regular,
		st_dle,
		st_bcode,
		st_semicolon1,
		st_digits1,
		st_semicolon2,
		st_digits2,
		st_digits3,
		st_error,
	};
	int st = st_regular, n = 0;
	uint8_t b, dle = 0;
	uint32_t i;
	bool nl = true;
	for (i = 0; i < log_data_len; i++){
		b = log_data[i];
		switch (st){
			case st_regular:
				if (b == 0x0a){
					printf("\r\n");
					nl = true;
				}else if (b == 0x0d){
					nl = false;
				}else if ((b == XPRN_DLE) || (b == XPRN_DLE1)){
					dle = b;
					st = st_dle;
				}else{
					if (b < 0x20)
						printf("\\x%.2hhx", b);
					else
						putchar(recode_win(b));
					nl = false;
				}
				break;
			case st_dle:
				if (b == XPRN_WR_BCODE){
					if (!nl)
						printf("\r\n");
					printf("╧╫. ╪╥╨╚╒: ");
					st = st_bcode;
				}else if (b == XPRN_RD_BCODE){
					if (!nl)
						printf("\r\n");
					printf("╫╥. ╪╥╨╚╒: ");
					st = st_bcode;
				}else if (b == XPRN_NO_BCODE){
					if (!nl)
						printf("\r\n");
					printf("┴┼╟ ╪╥╨╚╒\r\n");
					st = st_regular;
				}else{
					printf("\\x%.2hhx", dle);
					i--;
					st = st_regular;
				}
				nl = (b == XPRN_NO_BCODE);
				break;
			case st_bcode:
				if (b == ';')
					st = st_semicolon1;
				else if (is_digit(b)){
					n = 2;
					st = st_digits1;
				}else
					st = st_error;
				if (st != st_error)
					putchar(b);
				break;
			case st_semicolon1:
				if (b == ';')
					st = st_semicolon2;
				else if (is_digit(b)){
					n = 2;
					st = st_digits2;
				}else
					st = st_error;
				if (st != st_error)
					putchar(b);
				break;
			case st_digits1:
				if (--n > 0){
					if (is_digit(b))
						putchar(b);
					else
						st = st_error;
				}else{
					i--;
					st = st_semicolon1;
				}
				break;
			case st_semicolon2:
				if (is_digit(b)){
					putchar(b);
					n = 13;
					st = st_digits3;
				}else
					st = st_error;
				break;
			case st_digits2:
				if (--n > 0){
					if (is_digit(b))
						putchar(b);
					else
						st = st_error;
				}else{
					i--;
					st = st_semicolon2;
				}
				break;
			case st_digits3:
				if (--n > 0){
					if (is_digit(b))
						putchar(b);
					else
						st = st_error;
				}else{
					printf("\r\n");
					i--;
					st = st_regular;
				}
				break;
		}
		if (st == st_error){
			printf("\r\n");
			i--;
			nl = false;
			st = st_regular;
		}
	}
	if (st == st_digits3)
		printf("\r\n");
	printf("\r\n\r\n");
}

/* Вывод содержимого записей ЦКЛ за определённый промежуток времени */
static uint32_t write_recs(struct date_time *dt1, struct date_time *dt2)
{
	uint32_t ret = 0, idx;
	if (*((uint32_t *)dt2) < *((uint32_t *)dt1)){
		struct date_time tmp;
		*((uint32_t *)&tmp) = *((uint32_t *)dt1);
		*((uint32_t *)dt1) = *((uint32_t *)dt2);
		*((uint32_t *)dt2) = *((uint32_t *)&tmp);
	}
	idx = log_find_rec_by_date(hxlog, dt1);
	if (idx == -1UL)
		return 0;
	else if (!xlog_read_rec(hxlog, idx))
		return 0;
	while (*((uint32_t *)&xlog_rec_hdr.dt) < *((uint32_t *)dt1)){
		idx++;
		if ((idx >= hxlog->hdr->n_recs) ||
				!xlog_read_rec(hxlog, idx))
			break;
	}
	for (; idx < hxlog->hdr->n_recs; idx++){
		if (!xlog_read_rec(hxlog, idx))
			break;
		else if (*((uint32_t *)&xlog_rec_hdr.dt) > *((uint32_t *)dt2))
			break;
		else if (xlog_rec_hdr.type == XLRT_NORMAL){
			write_header();
			write_data();
			ret++;
		}
	}
	return ret;
}

/* Разбор строки вида ДД.ММ.ГГ-ЧЧ:мм */
static bool parse_dt(const char *s, struct date_time *dt)
{
	uint8_t day, mon, year, hour, min;
	bool ret = false;
	if ((strlen(s) == 14) &&
			(sscanf(s, "%2hhu.%2hhu.%2hhu-%2hhu:%2hhu",
				&day, &mon, &year, &hour, &min) == 5)){
		if ((day >= 1) && (day <= 31) &&
				(mon >= 1) && (mon <= 12) &&
				(year <= 63) &&
				(hour <= 23) && (min <= 59)){
			dt->year = year;
			dt->mon = mon - 1;
			dt->day = day - 1;
			dt->hour = hour;
			dt->min = min;
			dt->sec = 0;
			ret = true;
		}
	}
	return ret;
}

/* Разбор строки вида -h */
static bool parse_time_offs(const char *s, struct date_time *dt)
{
	char *p;
	int offs = strtol(s, &p, 10);
	if (*p == 0){
		time_t_to_date_time(time(NULL) + offs * 3600, dt);
		return true;
	}else
		return false;
}

/* Справка об использовании программы */
static void usage(void)
{
	fprintf(stderr,
		"Программа преобразования основной контрольной ленты терминала \"Экспресс-2А-К\"\n"
		"в текстовый вид. (c) gsr 2009.\n"
		"Использование:\ttxl [ДД1.ММ1.ГГ1-ЧЧ1.мм1 [ДД2.ММ2.ГГ2-ЧЧ2.мм2]] или\n"
		"\t\ttxl -ЧЧ.\n");
}

/*
 * Для избежания проблем с часовыми поясами к time(NULL) прибавляется
 * это смещение.
 */
#define NOW_OFFS	86400

int main(int argc, char **argv)
{
	int ret = 1;
	bool show_usage = false;
	struct date_time dt1, dt2;
	if (argc == 1){
		*((uint32_t *)&dt1) = 0;
		time_t_to_date_time(time(NULL) + NOW_OFFS, &dt2);
	}else if (argc == 2){
		if (argv[1][0] == '-'){
			if (parse_time_offs(argv[1], &dt1))
				time_t_to_date_time(time(NULL) + NOW_OFFS, &dt2);
			else{
				fprintf(stderr, "Неверный формат смещения "
					"начала интервала.\n");
				show_usage = true;
			}
		}else if (parse_dt(argv[1], &dt1))
			time_t_to_date_time(time(NULL) + NOW_OFFS, &dt2);
		else{
			fprintf(stderr, "Неверный формат даты начала интервала.\n");
			show_usage = true;
		}
	}else if (argc == 3){
		if (!parse_dt(argv[1], &dt1)){
			fprintf(stderr, "Неверный формат даты начала интервала.\n");
			show_usage = true;
		}else if (!parse_dt(argv[2], &dt2)){
			fprintf(stderr, "Неверный формат даты конца интервала.\n");
			show_usage = true;
		}
	}else{
		fprintf(stderr, "Слишком много параметров в командной строке.\n");
		show_usage = true;
	}
	if (show_usage /*+ (time(NULL) > 0x4a904030)*/)
		usage();
	else{
		hxlog->log_name = STERM_XLOG_OLD_NAME;
		if (log_open(hxlog, false)){
			uint32_t n = write_recs(&dt1, &dt2);
			fprintf(stderr, "Преобразовано записей: %u.\n", n);
			ret = 0;
		}else
			fprintf(stderr, "Ошибка открытия контрольной ленты терминала.\n");
	}
	return ret;
}

/* Заглушки для log/generic.c и log/express.c */
#include "md5.h"
#include "tki.h"

uint16_t term_check_sum;
time_t time_delta;
struct term_cfg cfg;
struct term_key_info tki;
struct md5_hash usb_bind;
struct md5_hash iplir_bind;
struct md5_hash iplir_check_sum;
struct md5_hash bank_license;
ds_number dsn;
int kt;
uint16_t oldZNtz;
uint16_t oldZNpo;
uint8_t oldZBp;
uint16_t ONpo;
uint16_t ONtz;
uint8_t OBp;
uint32_t reaction_time;

void flush_home(void)
{
}

bool is_escape(uint8_t c)
{
	return false;
}

char ds_key_char(int kt)
{
	return '*';
}

void get_tki_field(const struct term_key_info *info, int name, uint8_t *val)
{
}
