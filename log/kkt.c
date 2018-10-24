/* Функции для работы с контрольной лентой ККТ (ККЛ, КЛ3). (c) gsr 2018 */

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "kkt/kkt.h"
#include "log/kkt.h"
#include "cfg.h"
#include "express.h"
#include "paths.h"
#include "sterm.h"
#include "tki.h"

#define KLOG_MAP_MAX_SIZE	(KLOG_MAX_SIZE / sizeof(struct klog_rec_header))
static struct map_entry_t klog_map[KLOG_MAP_MAX_SIZE];

/* Заголовок контрольной ленты */
struct klog_header klog_hdr;
/* Заголовок текущей записи контрольной ленты */
struct klog_rec_header klog_rec_hdr;

/* Инициализация заголовка контрольной ленты (используется при ее создании) */
static void klog_init_hdr(struct log_handle *hlog)
{
	struct klog_header *hdr = (struct klog_header *)hlog->hdr;
	hdr->tag = KLOG_TAG;
	hdr->len = KLOG_MAX_SIZE;
	hdr->n_recs = 0;
	hdr->head = sizeof(*hdr);
	hdr->tail = sizeof(*hdr);
	hdr->cur_number = -1UL;
}

/* Вызывается при очистке контрольной ленты */
static void klog_clear(struct log_handle *hlog)
{
	struct klog_header *hdr = (struct klog_header *)hlog->hdr;
	hdr->head = hdr->tail;
	hdr->n_recs = 0;
	hdr->cur_number = -1UL;
}

/* CRC32 вычисляется по полиному 0x04c11db7 (IEEE 802.3). Для вычисления используется таблица */

static uint32_t crc32_tbl[256];
static bool crc32_tbl_ready = false;

static void init_crc32_tbl(void)
{
	for (uint32_t i = 0; i < 256; i++){
		uint32_t crc = i;
		for (int j = 0; j < 8; j++)
			crc = (crc & 1) ? (crc >> 1) ^ 0xedb88320 : crc >> 1;
		crc32_tbl[i] = crc;
	}
	crc32_tbl_ready = true;
}

static inline void init_crc32_tbl_if_need(void)
{
	if (!crc32_tbl_ready)
		init_crc32_tbl();
}

static uint32_t klog_rec_crc32(void)
{
	init_crc32_tbl_if_need();
	uint32_t crc32 = 0xffffffff;
	size_t len = sizeof(klog_rec_hdr) + klog_rec_hdr.len;
	for (int i = 0; i < len; i++){
		uint8_t b = (i < sizeof(klog_rec_hdr)) ?
			*((uint8_t *)&klog_rec_hdr + i) :
			log_data[i - sizeof(klog_rec_hdr)];
		crc32 = crc32_tbl[(crc32 ^ b) & 0xff] ^ (crc32 >> 8);
	}
	return crc32 ^= 0xffffffff;
}

/* Заполнение карты контрольной ленты */
static bool klog_fill_map(struct log_handle *hlog)
{
	struct klog_header *hdr = (struct klog_header *)hlog->hdr;
	memset(hlog->map, 0, hlog->map_size * sizeof(*hlog->map));
	hlog->map_head = 0;
	if (hdr->n_recs > KLOG_MAP_MAX_SIZE){
		fprintf(stderr, "Слишком много записей на %s; "
			"должно быть не более " _s(KLOG_MAP_MAX_SIZE) ".\n",
			hlog->log_type);
		return false;
	}
	for (uint32_t i = 0, offs = hdr->head; i < hdr->n_recs; i++){
		uint32_t tail = offs;
		try_fn(log_read(hlog, offs, (uint8_t *)&klog_rec_hdr, sizeof(klog_rec_hdr)));
		hlog->map[i].offset = offs;
		hlog->map[i].number = klog_rec_hdr.number;
		hlog->map[i].dt = klog_rec_hdr.dt;
		offs = log_inc_index(hlog, offs, sizeof(klog_rec_hdr));
		if (klog_rec_hdr.tag != KLOG_REC_TAG){
			fprintf(stderr, "Неверный формат заголовка записи %s #%u.\n",
				hlog->log_type, i);
			return log_truncate(hlog, i, tail);
		}else if (klog_rec_hdr.len > LOG_BUF_LEN){
			fprintf(stderr, "Слишком длинная запись %s #%u: %u байт (max %u).\n",
				hlog->log_type, i, klog_rec_hdr.len, LOG_BUF_LEN);
			return log_truncate(hlog, i, tail);
		}else if (klog_rec_hdr.len != (klog_rec_hdr.req_len + klog_rec_hdr.resp_len)){
			fprintf(stderr, "Неверные данные о длине записи %s #%u: %u != %u + %u.\n",
				hlog->log_type, i, klog_rec_hdr.len, klog_rec_hdr.req_len,
				klog_rec_hdr.resp_len);
			return log_truncate(hlog, i, tail);
		}
		log_data_len = klog_rec_hdr.len;
		if (log_data_len > 0)
			try_fn(log_read(hlog, offs, log_data, log_data_len));
		uint32_t crc = klog_rec_hdr.crc32;
		klog_rec_hdr.crc32 = 0;
		if (klog_rec_crc32() != crc){
			fprintf(stderr, "Несовпадение контрольной суммы для записи %s #%u.\n",
				hlog->log_type, i);
			return log_truncate(hlog, i, tail);
		}
		klog_rec_hdr.crc32 = crc;
		offs = log_inc_index(hlog, offs, klog_rec_hdr.len);
	}
	return true;
}

/* Чтение заголовка контрольной ленты */
static bool klog_read_header(struct log_handle *hlog)
{
	bool ret = false;
	if (log_atomic_read(hlog, 0, (uint8_t *)hlog->hdr, sizeof(struct klog_header))){
		if (hlog->hdr->tag != KLOG_TAG)
			fprintf(stderr, "Неверный формат заголовка %s.\n",
				hlog->log_type);
		else if (hlog->hdr->len > KLOG_MAX_SIZE)
			fprintf(stderr, "Неверный размер %s.\n", hlog->log_type);
		else{
			off_t len = lseek(hlog->rfd, 0, SEEK_END);
			if (len == (off_t)-1)
				fprintf(stderr, "Ошибка определения размера файла %s.\n",
					hlog->log_type);
			else{
				hlog->full_len = hlog->hdr->len + sizeof(struct klog_header);
				if (hlog->full_len == len)
					ret = true;
				else
					fprintf(stderr, "Размер %s, указанный в заголовке, не совпадает с реальным размером.\n",
						hlog->log_type);
			}
		}
	}
	return ret;
}

/* Запись заголовка контрольной ленты */
static bool klog_write_header(struct log_handle *hlog)
{
	return (hlog->wfd == -1) ? false : log_atomic_write(hlog, 0, (uint8_t *)hlog->hdr,
		sizeof(struct klog_header));
}

/* Структура для работы с контрольной лентой ККТ (ККЛ) */
static struct log_handle _hklog = {
	.hdr		= (struct log_header *)&klog_hdr,
	.hdr_len	= sizeof(klog_hdr),
	.full_len	= sizeof(klog_hdr),
	.log_type	= "ККЛ",
	.log_name	= STERM_KLOG_NAME,
	.rfd		= -1,
	.wfd		= -1,
	.map		= klog_map,
	.map_size	= KLOG_MAP_MAX_SIZE,
	.map_head	= 0,
	.on_open	= NULL,
	.on_close	= NULL,
	.init_log_hdr	= klog_init_hdr,
	.clear		= klog_clear,
	.fill_map	= klog_fill_map,
	.read_header	= klog_read_header,
	.write_header	= klog_write_header
};

struct log_handle *hklog = &_hklog;

/*
 * Определение индекса записи с заданным номером. Возвращает -1UL,
 * если запись не найдена.
 */
uint32_t klog_index_for_number(struct log_handle *hlog, uint32_t number)
{
	uint32_t i;
	for (i = 0; i < hlog->hdr->n_recs; i++){
		if (hlog->map[(hlog->map_head + i) % hlog->map_size].number == number)
			return i;
	}
	return -1UL;
}

/* Можно ли отпечатать диапазон записей контрольной ленты */
bool klog_can_print_range(struct log_handle *hlog)
{
	return cfg.has_xprn && (hlog->hdr->n_recs > 0);
}

/* Можно ли распечатать контрольную ленту полностью */
bool klog_can_print(struct log_handle *hlog)
{
	return klog_can_print_range(hlog);
}

/* Можно ли проводить поиск по контрольной ленте */
bool klog_can_find(struct log_handle *hlog)
{
	return hlog->hdr->n_recs > 0;
}

/*
 * Добавление новой записи в конец ККЛ. Данные находятся в log_data,
 * заголовок -- в klog_rec_hdr.
 */
static bool klog_add_rec(struct log_handle *hlog)
{
	uint32_t offs;
	uint32_t rec_len = klog_rec_hdr.len + sizeof(klog_rec_hdr);
	if (rec_len > hlog->hdr->len){
		fprintf(stderr, "Длина записи (%u байт) превышает длину %s (%u байт).\n",
				rec_len, hlog->log_type, hlog->hdr->len);
		return false;
	}
/* Если при записи на циклическую ККЛ не хватает места, удаляем записи в начале */
	uint32_t free_len = log_free_space(hlog), tail = 0, n, m = hlog->map_head;
	for (n = hlog->hdr->n_recs; (n > 0) && (free_len < rec_len); n--){
		if (n == 1)
			tail = hlog->hdr->tail;
		else
			tail = hlog->map[(m + 1) % hlog->map_size].offset;
		free_len += log_delta(hlog, tail, hlog->map[m].offset);
		m++;
		m %= hlog->map_size;
	}
	if (free_len < rec_len){
		fprintf(stderr, "Не удалось освободить место под новую запись.\n");
		return false;
	}
/* n -- оставшееся число записей на контрольной ленте */
	if ((n + 1) > hlog->map_size){
		fprintf(stderr, "Превышено максимальное число записей на %s;\n"
			"должно быть не более %u.\n", hlog->log_type, hlog->map_size);
		return false;
	}
/* Начинаем запись на контрольную ленту */
	bool ret = false;
	if (log_begin_write(hlog)){
		hlog->map_head = m;
		tail = (hlog->map_head + n) % hlog->map_size;
		hlog->map[tail].offset = hlog->hdr->tail;
		hlog->map[tail].number = klog_rec_hdr.number;
		hlog->map[tail].dt = klog_rec_hdr.dt;
		hlog->hdr->n_recs = n + 1;
		hlog->hdr->head = hlog->map[hlog->map_head].offset;
		offs = hlog->hdr->tail;
		hlog->hdr->tail = log_inc_index(hlog, hlog->hdr->tail,
				sizeof(klog_rec_hdr) + klog_rec_hdr.len);
		ret = hlog->write_header(hlog) &&
			log_write(hlog, offs, (uint8_t *)&klog_rec_hdr, sizeof(klog_rec_hdr)) &&
			((klog_rec_hdr.len == 0) ||
			log_write(hlog, log_inc_index(hlog, offs, sizeof(klog_rec_hdr)),
				log_data, klog_rec_hdr.len));
	}
	log_end_write(hlog);
	return ret;
}

static inline uint64_t timeb_to_ms(const struct timeb *t)
{
	return t->time * 1000 + t->millitm;
}

static uint32_t get_op_time(const struct timeb *t0)
{
	struct timeb t;
	ftime(&t);
	return (uint32_t)(timeb_to_ms(&t) - timeb_to_ms(t0));
}

/* Заполнение заголовка записи контрольной ленты */
static void klog_init_rec_hdr(struct log_handle *hlog, struct klog_rec_header *hdr,
	const struct timeb *t0)
{
	hdr->tag = KLOG_REC_TAG;
	hlog->hdr->cur_number++;
	hdr->number = hlog->hdr->cur_number;
	hdr->stream = KLOG_STREAM_CTL;
	hdr->len = 0;
	hdr->req_len = hdr->resp_len = 0;
	time_t_to_date_time(t0->time, &hdr->dt);
	hdr->ms = t0->millitm;
	hdr->op_time = get_op_time(t0);
	hdr->addr.gaddr = cfg.gaddr;
	hdr->addr.iaddr = cfg.iaddr;
	hdr->term_version = STERM_VERSION;
	hdr->term_check_sum = term_check_sum;
	get_tki_field(&tki, TKI_NUMBER, (uint8_t *)hdr->tn);
	memcpy(hdr->kkt_nr, kkt_nr, sizeof(hdr->kkt_nr));
	memcpy(hdr->dsn, dsn, DS_NUMBER_LEN);
	hdr->ds_type = kt;
	hdr->cmd = KKT_NUL;
	hdr->status = KKT_STATUS_OK;
	hdr->crc32 = 0;
}

static bool status_ok(uint8_t prefix, uint8_t cmd, uint8_t status)
{
	bool ret = false;
	switch (prefix){
		case KKT_POLL:
			switch (cmd){
				case KKT_POLL_ENQ:
				case KKT_POLL_PARAMS:
					ret = status == KKT_STATUS_OK;
					break;
			}
			break;
		case KKT_SRV:
			switch (cmd){
				case KKT_SRV_FDO_IFACE:
					ret = status == FDO_IFACE_STATUS_OK;
					break;
				case KKT_SRV_FDO_ADDR:
					ret = status == FDO_ADDR_STATUS_OK;
					break;
				case KKT_SRV_FDO_REQ:
					ret = status < KKT_STATUS_INTERNAL_BASE;
					break;
				case KKT_SRV_FDO_DATA:
					ret = status == FDO_DATA_STATUS_OK;
					break;
				case KKT_SRV_FLOW_CTL:
					ret = status == FLOW_CTL_STATUS_OK;
					break;
				case KKT_SRV_RST_TYPE:
					ret = status == RST_TYPE_STATUS_OK;
					break;
				case KKT_SRV_RTC_GET:
					ret = status == RTC_GET_STATUS_OK;
					break;
				case KKT_SRV_RTC_SET:
					ret = status == RTC_SET_STATUS_OK;
					break;
				case KKT_SRV_LAST_DOC_INFO:
				case KKT_SRV_PRINT_LAST:
				case KKT_SRV_BEGIN_DOC:
				case KKT_SRV_SEND_DOC:
				case KKT_SRV_END_DOC:
					ret = status == KKT_STATUS_OK;
					break;
				case KKT_SRV_NET_SETTINGS:
					ret = status == NET_SETTINGS_STATUS_OK;
					break;
				case KKT_SRV_GPRS_SETTINGS:
					ret = status == GPRS_CFG_STATUS_OK;
					break;
			}
			break;
		case KKT_FS:
			switch (cmd){
				case KKT_FS_STATUS:
				case KKT_FS_NR:
				case KKT_FS_LIFETIME:
				case KKT_FS_VERSION:
				case KKT_FS_LAST_ERROR:
				case KKT_FS_SHIFT_PARAMS:
				case KKT_FS_TRANSMISSION_STATUS:
				case KKT_FS_FIND_DOC:
				case KKT_FS_FIND_FDO_ACK:
				case KKT_FS_UNCONFIRMED_DOC_CNT:
				case KKT_FS_REGISTRATION_RESULT:
				case KKT_FS_REGISTRATION_RESULT2:
				case KKT_FS_REGISTRATION_PARAM:
				case KKT_FS_REGISTRATION_PARAM2:
				case KKT_FS_GET_DOC_STLV:
				case KKT_FS_READ_DOC_TLV:
				case KKT_FS_READ_REGISTRATION_TLV:
				case KKT_FS_LAST_REG_DATA:
				case KKT_FS_RESET:
					ret = status == KKT_STATUS_OK;
					break;
			}
			break;
		case KKT_NUL:
			case KKT_LOG:
			case KKT_VF:
			case KKT_CHEQUE:
				ret = status == KKT_STATUS_OK;
				break;
	}
	return ret;
}

static int get_kkt_stream(uint8_t prefix, uint8_t cmd)
{
	int ret = KLOG_STREAM_CTL;
	switch (prefix){
		case KKT_POLL:
		case KKT_FS:
			break;
		case KKT_SRV:
			switch (cmd){
				case KKT_SRV_FDO_REQ:
				case KKT_SRV_FDO_DATA:
					ret = KLOG_STREAM_FDO;
			}
			break;
		case 0:
			if (cmd != KKT_ECHO)
				ret = KLOG_STREAM_PRN;
			break;
	}
	return ret;
}


/* Занесение записи на ККЛ. Возвращает номер записи */
uint32_t klog_write_rec(struct log_handle *hlog, const struct timeb *t0,
	const uint8_t *req, uint16_t req_len,
	uint8_t status, const uint8_t *resp, uint16_t resp_len)
{
	if ((hlog == NULL) || (t0 == NULL) || (req == NULL) || (req_len < 2) ||
			(resp == NULL) || (resp_len < 2))
		return -1UL;
	size_t len = req_len + resp_len;
	if (len > sizeof(log_data)){
		fprintf(stderr, "Переполнение буфера данных при записи на %s (%u байт).\n",
			hlog->log_type, len);
		return -1UL;
	}else if (cfg.kkt_log_level == KLOG_LEVEL_OFF)
		return -1UL;
	uint8_t cmd = req[1], prefix = KKT_NUL;
	if ((cmd == KKT_POLL) || (cmd == KKT_SRV) || (cmd == KKT_FS)){
		if (req_len < 3)
			return -1UL;
		else{
			prefix = cmd;
			cmd = req[2];
		}
	}
	if (status_ok(prefix, cmd, status) &&
			((cfg.kkt_log_level == KLOG_LEVEL_WARN) ||
			 (cfg.kkt_log_level == KLOG_LEVEL_ERR)))
		return -1UL;
	int stream = get_kkt_stream(prefix, cmd);
	log_data_len = log_data_index = 0;
	memcpy(log_data, req, req_len);
	memcpy(log_data + req_len, resp, resp_len);
	log_data_len = len;
	klog_init_rec_hdr(hlog, &klog_rec_hdr, t0);
	klog_rec_hdr.stream = stream;
	klog_rec_hdr.len = len;
	klog_rec_hdr.req_len = req_len;
	klog_rec_hdr.resp_len = resp_len;
	klog_rec_hdr.cmd = cmd;
	klog_rec_hdr.status = status;
	klog_rec_hdr.crc32 = klog_rec_crc32();
	return klog_add_rec(hlog) ? klog_rec_hdr.number : -1UL;
}

/* Чтение заданной записи контрольной ленты */
bool klog_read_rec(struct log_handle *hlog, uint32_t index)
{
	if (index >= hlog->hdr->n_recs)
		return false;
	log_data_len = log_data_index = 0;
	uint32_t offs = hlog->map[(hlog->map_head + index) % hlog->map_size].offset;
	if (!log_read(hlog, offs, (uint8_t *)&klog_rec_hdr, sizeof(klog_rec_hdr)))
		return false;
	if (klog_rec_hdr.len > sizeof(log_data)){
		fprintf(stderr, "Слишком длинная запись %s #%u (%u байт).\n",
				hlog->log_type, index, klog_rec_hdr.len);
		return false;
	}
	offs = log_inc_index(hlog, offs, sizeof(klog_rec_hdr));
	if ((klog_rec_hdr.len > 0) && !log_read(hlog, offs, log_data, klog_rec_hdr.len))
		return false;
	uint32_t crc = klog_rec_hdr.crc32;
	klog_rec_hdr.crc32 = 0;
	if (klog_rec_crc32() != crc){
		fprintf(stderr, "Несовпадение контрольной суммы для записи %s #%u.\n",
				hlog->log_type, index);
		return false;
	}
	klog_rec_hdr.crc32 = crc;
	log_data_len = klog_rec_hdr.len;
	return true;
}

/* Вывод заголовка распечатки ККЛ */
bool klog_print_header(void)
{
	try_fn(prn_write_str("\x1e\x1eНАЧАЛО ПЕЧАТИ КОНТРОЛЬНОЙ ЛЕНТЫ "
		"(РАБОТА С ККТ) "));
	try_fn(prn_write_cur_date_time());
	return prn_write_str("\x1c\x0b") && prn_write_eol();
}

/* Вывод концевика распечатки контрольной ленты */
bool klog_print_footer(void)
{
	log_reset_prn_buf();
	try_fn(prn_write_str("\x1e\x1e\x10\x0bКОНЕЦ ПЕЧАТИ КОНТРОЛЬНОЙ ЛЕНТЫ "
		"(РАБОТА С ККТ) "));
	try_fn(prn_write_cur_date_time());
	return prn_write_str("\x1c\x0b") && prn_write_eol();
}

/* Вывод заголовка записи на печать */
static bool klog_print_rec_header(void)
{
	return 	prn_write_str_fmt("\x1e%.2hhX%.2hhX (\"ЭКСПРЕСС-2А-К\" "
			"%hhu.%hhu.%hhu %.4hX %.*s) ККТ=%.*s",
			klog_rec_hdr.addr.gaddr, klog_rec_hdr.addr.iaddr,
			VERSION_BRANCH(klog_rec_hdr.term_version),
			VERSION_RELEASE(klog_rec_hdr.term_version),
			VERSION_PATCH(klog_rec_hdr.term_version),
			klog_rec_hdr.term_check_sum,
			sizeof(klog_rec_hdr.tn), klog_rec_hdr.tn,
			sizeof(klog_rec_hdr.kkt_nr), klog_rec_hdr.kkt_nr) &&
		prn_write_eol() &&
		prn_write_str_fmt("ЗАПИСЬ %u ОТ ", klog_rec_hdr.number + 1) &&
		prn_write_date_time(&klog_rec_hdr.dt) &&
		prn_write_str_fmt(" ЖЕТОН %c %.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX%.2hhX",
			ds_key_char(klog_rec_hdr.ds_type),
			klog_rec_hdr.dsn[7], klog_rec_hdr.dsn[0],
			klog_rec_hdr.dsn[6], klog_rec_hdr.dsn[5],
			klog_rec_hdr.dsn[4], klog_rec_hdr.dsn[3],
			klog_rec_hdr.dsn[2], klog_rec_hdr.dsn[1]) &&
		prn_write_eol();
}

static inline bool is_print(uint8_t b)
{
	return (b >= 0x20) && (b <= 0x7f);
}

/* Вывод на печать строки контрольной ленты */
static bool klog_print_line(bool recode)
{
	if (log_data_index >= klog_rec_hdr.len)
		return false;
	uint32_t begin = log_data_index, end = klog_rec_hdr.req_len, addr = begin;
	if (log_data_index >= klog_rec_hdr.resp_len){
		end = klog_rec_hdr.len;
		addr -= klog_rec_hdr.req_len;
	}
	static char ln[128];
	sprintf(ln, "%.4X:", addr);
	off_t offs = 5;
	for (uint32_t i = 0, idx = begin; i < 16; i++, idx++, offs += 3){
		if ((i != 0) && ((i % 8) == 0)){
			sprintf(ln + offs, " --");
			offs += 3;
		}
		if (idx < end)
			sprintf(ln + offs, " %.2hhX", log_data[idx]);
		else
			sprintf(ln + offs, "   ");
	}
	sprintf(ln + offs, "   ");
	offs += 3;
	for (uint32_t i = 0, idx = begin; i < 16; i++, idx++, offs++){
		if (idx < end){
			uint8_t b = log_data[idx];
			sprintf(ln + offs, "%c", is_print(b) ? b : ' ');
		}else
			sprintf(ln + offs, " ");
	}
	bool ret = recode ? prn_write_str(ln) : prn_write_chars_raw(ln, -1);
	return ret && prn_write_eol();
}

static bool klog_print_delim(size_t len)
{
	bool ret = true;
	for (int i = 0; i < len; i++){
		ret = prn_write_char_raw('=');
		if (!ret)
			break;
	}
	return ret && prn_write_eol();
}

static bool klog_print_data_header(const char *title)
{
	bool ret = true;
	if (title != NULL)
		ret = prn_write_str(title) && prn_write_eol();
	return ret && klog_print_delim(76);
}

/* Вывод записи контрольной ленты на печать */
bool klog_print_rec(void)
{
	log_reset_prn_buf();
	try_fn(klog_print_rec_header());
	bool recode = klog_rec_hdr.stream != KLOG_STREAM_PRN;
	if (klog_rec_hdr.req_len > 0){
		try_fn(klog_print_data_header("ЗАПРОС:"));
		for (log_data_index = 0; log_data_index < klog_rec_hdr.req_len;
				log_data_index += 16)
			try_fn(klog_print_line(recode));
	}
	if (klog_rec_hdr.resp_len > 0){
		try_fn(klog_print_data_header("ОТВЕТ:"));
		for (log_data_index = klog_rec_hdr.req_len; log_data_index < log_data_len;
				log_data_index += 16)
			try_fn(klog_print_line(recode));
	}
	return true;
}
