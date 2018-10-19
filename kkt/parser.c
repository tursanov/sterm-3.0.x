/* ������ �⢥⮢ ���. (c) gsr 2018 */

#include <assert.h>
#include <string.h>
#include "kkt/io.h"
#include "kkt/parser.h"

uint8_t tx_prefix = KKT_NUL;
uint8_t tx_cmd = KKT_NUL;

uint8_t kkt_status = KKT_STATUS_OK;

static enum {
	st_prefix,
	st_fix_data,
	st_var_data_len,
	st_var_data,
} rx_st = st_prefix;

void begin_rx(void)
{
	reset_rx();
	rx_exp_len = (tx_prefix == KKT_NUL) ? 3 : 4;
	rx_st = st_prefix;
}

static inline void begin_fix_data(size_t len)
{
	rx_exp_len += len;
	rx_st = st_fix_data;
}

static inline void begin_var_data_len(void)
{
	rx_exp_len += 2;
	rx_st = st_var_data_len;
}

static inline void begin_var_data(size_t len)
{
	rx_exp_len += len;
	rx_st = st_var_data;
}

static void parse_date(const uint8_t *data, struct kkt_fs_date *date)
{
	date->year = from_bcd(*data++) + 2000;
	date->month = from_bcd(*data++);
	date->day = from_bcd(*data);
}

static void parse_time(const uint8_t *data, struct kkt_fs_time *time)
{
	time->hour = from_bcd(*data++);
	time->minute = from_bcd(*data++);
}

static void parse_date_time(const uint8_t *data, struct kkt_fs_date_time *dt)
{
	parse_date(data, &dt->date);
	parse_time(data + KKT_FS_DATE_LEN, &dt->time);
}

static bool parse_prefix(void)
{
	bool ret = false;
	if ((rx_len > 1) && (rx[0] == KKT_ESC2)){
		if (tx_prefix == KKT_NUL){
			if ((rx_len == 3) && (rx[1] == tx_cmd)){
				kkt_status = rx[2];
				ret = true;
			}
		}else if ((rx_len == 4) && (rx[1] == tx_prefix) && (rx[2] == tx_cmd)){
			kkt_status = rx[3];
			ret = true;
		}
	}
	return ret;
}

static bool parse_status(void)
{
	return parse_prefix();
}

static void parse_var_data_len(struct kkt_var_data *data)
{
	data->len = *(uint16_t *)(rx + rx_len - 2);
	if (data->len > 0){
		data->data = rx + rx_len;
		begin_var_data(data->len);
	}else
		data->data = NULL;
}

static bool parse_var_data(struct kkt_var_data *data)
{
	assert(data != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret)
			begin_var_data_len();
	}else if (rx_st == st_var_data_len)
		parse_var_data_len(data);
	return ret;
}

static bool parse_fdo_req(struct kkt_fdo_cmd *fdo_cmd)
{
	assert(fdo_cmd != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret){
			fdo_cmd->cmd = kkt_status;
			begin_var_data_len();
		}
	}else if (rx_st == st_var_data_len)
		parse_var_data_len(&fdo_cmd->data);
	return ret;
}

static bool check_rtc_data(const struct kkt_rtc_data *rtc)
{
	bool ret = true;
	static uint8_t mdays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if ((rtc->year < 2018) || (rtc->month < 1) || (rtc->month > 12) || (rtc->day < 1))
		ret = false;
	else if ((rtc->month == 2) && ((rtc->year % 4) == 0) &&
			(((rtc->year % 100) != 0) || ((rtc->year % 400) == 0)) &&
			(rtc->day > 29))
		ret = false;
	else if (rtc->day > mdays[rtc->month])
		ret = false;
	else if ((rtc->hour > 23) || (rtc->minute > 59) || (rtc->second > 59))
		ret = false;
	return ret;
}

static bool parse_rtc_data(struct kkt_rtc_data *rtc)
{
	assert(rtc != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == RTC_GET_STATUS_OK))
			begin_fix_data(7);
	}else if (rx_st == st_fix_data){
		memset(rtc, 0, sizeof(rtc));
		if (is_bcd(rx[4]) && is_bcd(rx[5]))
			rtc->year = from_bcd(rx[4]) * 100 + from_bcd(rx[5]);
		else
			ret = false;
		if (ret && is_bcd(rx[6]))
			rtc->month = from_bcd(rx[6]);
		else
			ret = false;
		if (ret && is_bcd(rx[7]))
			rtc->day = from_bcd(rx[7]);
		else
			ret = false;
		if (ret && is_bcd(rx[8]))
			rtc->hour = from_bcd(rx[8]);
		else
			ret = false;
		if (ret && is_bcd(rx[9]))
			rtc->minute = from_bcd(rx[9]);
		else
			ret = false;
		if (ret && is_bcd(rx[10]))
			rtc->second = from_bcd(rx[10]);
		else
			ret = false;
		if (ret)
			ret = check_rtc_data(rtc);
	}
	return ret;
}

static bool parse_last_doc_info(struct last_doc_info_arg *arg)
{
	assert(arg != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret){
			if (kkt_status == KKT_STATUS_OK)
				begin_fix_data(12);
			else{
				arg->ldi.last_nr = arg->ldi.last_printed_nr = 0;
				arg->ldi.last_type = arg->ldi.last_printed_type = 0;
				begin_var_data_len();
			}
		}
	}else if (rx_st == st_fix_data){
		off_t offs = rx_len - 12;
		arg->ldi.last_nr = *(const uint32_t *)(rx + offs);
		offs += sizeof(uint32_t);
		arg->ldi.last_type = *(const uint16_t *)(rx + offs);
		offs += sizeof(uint16_t);
		arg->ldi.last_printed_nr = *(const uint32_t *)(rx + offs);
		offs += sizeof(uint32_t);
		arg->ldi.last_printed_type = *(const uint16_t *)(rx + offs);
		begin_var_data_len();
	}else if (rx_st == st_var_data_len)
		parse_var_data_len(&arg->err_info);
	return ret;
}

static bool parse_print_last_doc(struct last_printed_info_arg *arg)
{
	assert(arg != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret){
			if (kkt_status == KKT_STATUS_OK)
				begin_fix_data(10);
			else{
				arg->lpi.nr = 0;
				arg->lpi.sign = 0;
				arg->lpi.type = 0;
				begin_var_data_len();
			}
		}
	}else if (rx_st == st_fix_data){
		off_t offs = rx_len - 10;
		arg->lpi.nr = *(const uint32_t *)(rx + offs);
		offs += sizeof(uint32_t);
		arg->lpi.sign = *(const uint32_t *)(rx + offs);
		offs += sizeof(uint32_t);
		arg->lpi.type = *(const uint16_t *)(rx + offs);
		begin_var_data_len();
	}else if (rx_st == st_var_data_len)
		parse_var_data_len(&arg->err_info);
	return ret;
}

static bool parse_end_doc(struct doc_info_arg *arg)
{
	assert(arg != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret){
			if (kkt_status == KKT_STATUS_OK)
				begin_fix_data(8);
			else{
				arg->di.nr = 0;
				arg->di.sign = 0;
				begin_var_data_len();
			}
		}
	}else if (rx_st == st_fix_data){
		off_t offs = rx_len - 8;
		arg->di.nr = *(const uint32_t *)(rx + offs);
		offs += sizeof(uint32_t);
		arg->di.sign = *(const uint32_t *)(rx + offs);
		begin_var_data_len();
	}else if (rx_st == st_var_data_len)
		parse_var_data_len(&arg->err_info);
	return ret;
}

static bool parse_fs_status(struct kkt_fs_status *fs_status)
{
	assert(fs_status != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(30);
	}else if (rx_st == st_fix_data){
		off_t offs = rx_len - 30;
		fs_status->life_state = rx[offs++];
		fs_status->current_doc = rx[offs++];
		fs_status->doc_data = rx[offs++];
		fs_status->shift_state = rx[offs++];
		fs_status->alert_flags = rx[offs++];
		parse_date_time(rx + offs, &fs_status->dt);
		offs += KKT_FS_DATE_TIME_LEN;
		memcpy(fs_status->nr, rx + offs, KKT_FS_NR_LEN);
		fs_status->nr[KKT_FS_NR_LEN] = 0;
		offs += KKT_FS_NR_LEN;
		fs_status->last_doc_nr = *(const uint32_t *)(rx + offs);
	}
	return ret;
}

static bool parse_fs_nr(char *fs_nr)
{
	assert(fs_nr != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(KKT_FS_NR_LEN);
	}else if (rx_st == st_fix_data){
		memcpy(fs_nr, rx + rx_len - KKT_FS_NR_LEN, KKT_FS_NR_LEN);
		fs_nr[KKT_FS_NR_LEN] = 0;
	}
	return ret;
}

static bool parse_fs_lifetime(struct kkt_fs_lifetime *lt)
{
	assert(lt != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(7);
	}else if (rx_st == st_fix_data){
		off_t offs = rx_len - 7;
		parse_date_time(rx + offs, &lt->dt);
		offs += KKT_FS_DATE_TIME_LEN;
		lt->reg_remain = rx[offs++];
		lt->reg_complete = rx[offs];
	}
	return ret;
}

static bool parse_fs_version(struct kkt_fs_version *ver)
{
	assert(ver != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(KKT_FS_VERSION_LEN + 1);
	}else if (rx_st == st_fix_data){
		off_t offs = rx_len - KKT_FS_VERSION_LEN - 1;
		memcpy(ver->version, rx + offs, KKT_FS_VERSION_LEN);
		ver->version[KKT_FS_VERSION_LEN] = 0;
		offs += KKT_FS_VERSION_LEN;
		ver->type = rx[offs];
	}
	return ret;
}

static bool parse_fs_last_error(struct kkt_var_data *err_info)
{
	assert(err_info != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_var_data_len();
	}else if (rx_st == st_var_data_len)
		parse_var_data_len(err_info);
	return ret;
}

static bool parse_fs_shift_state(struct kkt_fs_shift_state *ss)
{
	assert(ss != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(5);
	}else if (rx_st == st_fix_data){
		off_t offs = rx_len - 5;
		ss->opened = (rx[offs++] != 0);
		ss->shift_nr = *(const uint16_t *)(rx + offs);
		offs += sizeof(uint16_t);
		ss->cheque_nr = *(const uint16_t *)(rx + offs);
	}
	return ret;
}

static bool parse_fs_transmission_state(struct kkt_fs_transmission_state *ts)
{
	assert(ts);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(13);
	}else if (rx_st == st_fix_data){
		off_t offs = rx_len - 13;
		ts->state = rx[offs++];
		ts->read_msg_started = (rx[offs++] != 0);
		ts->sndq_len = *(const uint16_t *)(rx + offs);
		offs += sizeof(uint16_t);
		ts->first_doc_nr = *(const uint32_t *)(rx + offs);
		offs += sizeof(uint32_t);
		parse_date_time(rx + offs, &ts->first_doc_dt);
	}
	return ret;
}

static size_t get_doc_data_len(uint8_t doc_type)
{
	size_t ret = 0;
	switch (doc_type){
		case REGISTRATION:
			ret = sizeof(struct kkt_register_report);
			break;
		case RE_REGISTRATION:
			ret = sizeof(struct kkt_reregister_report);
			break;
		case OPEN_SHIFT:
		case CLOSE_SHIFT:
			ret = sizeof(struct kkt_shift_report);
			break;
		case CALC_REPORT:
			ret = sizeof(struct kkt_calc_report);
			break;
		case CHEQUE:
		case CHEQUE_CORR:
		case BSO:
		case BSO_CORR:
			ret = sizeof(struct kkt_cheque_report);
			break;
		case CLOSE_FS:
			ret = sizeof(struct kkt_close_fs);
			break;
		case OP_COMMIT:
			ret = sizeof(struct kkt_fdo_ack);
			break;
	}
	return ret;
}

static bool parse_fs_find_doc(struct find_doc_info_arg *arg)
{
	assert(arg != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(2);
	}else if (rx_st == st_fix_data){
		arg->fdi.doc_type = rx[rx_len - 2];
		arg->fdi.fdo_ack = rx[rx_len - 1];
		arg->data.len = get_doc_data_len(arg->fdi.doc_type);
		if (arg->data.len > 0){
			arg->data.data = rx + rx_len;
			begin_var_data(arg->data.len);
		}else
			arg->data.data = NULL;
	}
	return ret;
}

static bool parse_fs_fdo_ack(struct kkt_fs_fdo_ack *fdo_ack)
{
	assert(fdo_ack != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(sizeof(struct kkt_fdo_ack));

	}else if (rx_st == st_fix_data){
		off_t offs = rx_len - sizeof(struct kkt_fdo_ack);
		parse_date_time(rx + offs, &fdo_ack->dt);
		offs += KKT_FS_DATE_TIME_LEN;
		memcpy(fdo_ack->fiscal_sign, rx + offs, sizeof(fdo_ack->fiscal_sign));
		offs += sizeof(fdo_ack->fiscal_sign);
		fdo_ack->doc_nr = *(const uint32_t *)(rx + offs);
	}
	return ret;
}

static bool parse_fs_unconfirmed_cnt(uint32_t *cnt)
{
	assert(cnt != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(sizeof(uint16_t));
	}else if (rx_st == st_fix_data)
		*cnt = *(const uint16_t *)(rx + rx_len - sizeof(uint16_t));
	return ret;
}

parser_t get_parser(uint8_t prefix, uint8_t cmd)
{
	parser_t ret = NULL;
	switch (prefix){
		case KKT_SRV:
			switch (cmd){
				case KKT_SRV_FDO_IFACE:
				case KKT_SRV_FDO_ADDR:
				case KKT_SRV_FDO_DATA:
				case KKT_SRV_FLOW_CTL:
				case KKT_SRV_RST_TYPE:
				case KKT_SRV_RTC_SET:
				case KKT_SRV_NET_SETTINGS:
				case KKT_SRV_GPRS_SETTINGS:
					ret = parse_status;
					break;
				case KKT_SRV_FDO_REQ:
					ret = parse_fdo_req;
					break;
				case KKT_SRV_RTC_GET:
					ret = parse_rtc_data;
					break;
				case KKT_SRV_LAST_DOC_INFO:
					ret = parse_last_doc_info;
					break;
				case KKT_SRV_PRINT_LAST:
					ret = parse_print_last_doc;
					break;
				case KKT_SRV_BEGIN_DOC:
				case KKT_SRV_SEND_DOC:
					ret = parse_var_data;
					break;
				case KKT_SRV_END_DOC:
					ret = parse_end_doc;
					break;

			}
			break;
		case KKT_FS:
			switch (cmd){
				case KKT_FS_STATUS:
					ret = parse_fs_status;
					break;
				case KKT_FS_NR:
					ret = parse_fs_nr;
					break;
				case KKT_FS_LIFETIME:
					ret = parse_fs_lifetime;
					break;
				case KKT_FS_VERSION:
					ret = parse_fs_version;
					break;
				case KKT_FS_LAST_ERROR:
					ret = parse_fs_last_error;
					break;
				case KKT_FS_SHIFT_PARAMS:
					ret = parse_fs_shift_state;
					break;
				case KKT_FS_TRANSMISSION_STATUS:
					ret = parse_fs_transmission_state;
					break;
				case KKT_FS_FIND_DOC:
					ret = parse_fs_find_doc;
					break;
				case KKT_FS_FIND_FDO_ACK:
					ret = parse_fs_fdo_ack;
					break;
				case KKT_FS_UNCONFIRMED_DOC_CNT:
					ret = parse_fs_unconfirmed_cnt;
					break;
				case KKT_FS_LAST_REG_DATA:
					ret = parse_var_data;
					break;
			}
			break;
	}
	return ret;
}
