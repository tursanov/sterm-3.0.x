/* Основной модуль для работы с ККТ. (c) gsr 2018 */

#include <sys/timeb.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "kkt/io.h"
#include "kkt/kkt.h"
#include "kkt/parser.h"
#include "log/kkt.h"
#include "cfg.h"

const struct dev_info *kkt = NULL;

static char kkt_nr_buf[KKT_NR_LEN + 1];
const char *kkt_nr = NULL;

static char kkt_version_buf[10];
const char *kkt_ver = NULL;

static char kkt_fs_nr_buf[KKT_FS_NR_LEN + 1];
const char *kkt_fs_nr = NULL;

static void set_fs_nr(const char *fs_nr)
{
	if (fs_nr == NULL)
		kkt_fs_nr = NULL;
	else{
		snprintf(kkt_fs_nr_buf, sizeof(kkt_fs_nr_buf), "%s", fs_nr);
		kkt_fs_nr = kkt_fs_nr_buf;
	}
}

void kkt_init(const struct dev_info *di)
{
	kkt_io_init(di);
	const char *s = get_dev_param_str(di, KKT_SN);
	if (s == NULL)
		kkt_nr = NULL;
	else{
		snprintf(kkt_nr_buf, sizeof(kkt_nr_buf), "%s", s);
		kkt_nr = kkt_nr_buf;
	}
	s = get_dev_param_str(di, KKT_VERSION);
	if (s == NULL)
		kkt_ver = NULL;
	else{
		snprintf(kkt_version_buf, sizeof(kkt_version_buf), "%s", s);
		kkt_ver = kkt_version_buf;
	}
}

void kkt_release(void)
{
	kkt_io_release();
	kkt_nr = NULL;
	kkt_ver = NULL;
	kkt_fs_nr = NULL;
}

static bool on_wr_overflow(void)
{
	kkt_status = KKT_STATUS_WR_OVERFLOW;
	return false;
}

static inline bool write_byte(uint8_t b)
{
	bool ret = true;
	if (likely(kkt_tx_len < sizeof(kkt_tx)))
		kkt_tx[kkt_tx_len++] = b;
	else
		ret = on_wr_overflow();
	return ret;
}

static inline bool write_quot(void)
{
	return write_byte(0x22);
}

static inline bool write_word(uint16_t w)
{
	bool ret = true;
	if (likely((kkt_tx_len + sizeof(w)) <= sizeof(kkt_tx))){
		*(uint16_t *)(kkt_tx + kkt_tx_len) = w;
		kkt_tx_len += sizeof(uint16_t);
	}else
		ret = on_wr_overflow();
	return ret;
}

static inline bool write_dword(uint32_t dw)
{
	bool ret = true;
	if (likely((kkt_tx_len + sizeof(dw)) <= sizeof(kkt_tx))){
		*(uint32_t *)(kkt_tx + kkt_tx_len) = dw;
		kkt_tx_len += sizeof(uint32_t);
	}else
		ret = on_wr_overflow();
	return ret;
}

static inline bool write_data(const uint8_t *data, size_t len)
{
	bool ret = true;
	if (likely((data != NULL) && (len > 0) && ((kkt_tx_len + len) <= sizeof(kkt_tx)))){
		memcpy(kkt_tx + kkt_tx_len, data, len);
		kkt_tx_len += len;
	}else
		ret = on_wr_overflow();
	return ret;
}

static bool write_cmd(uint8_t cmd)
{
	return write_byte(KKT_ESC) && write_byte(cmd);
}

static bool write_cmd2(uint8_t prefix, uint8_t cmd)
{
	return write_cmd(prefix) && write_byte(cmd);
}

static bool write_bcd(uint32_t v, size_t n)
{
	if (unlikely((n == 0) || (n > 4)))
		return false;
	else if (unlikely((kkt_tx_len + n) > sizeof(kkt_tx)))
		return on_wr_overflow();
	for (int i = 1; i <= n; i++){
		kkt_tx[kkt_tx_len + n - i] = to_bcd(v % 100);
		v /= 100;
	}
	kkt_tx_len += n;
	return true;
}

static bool write_uint(uint32_t v, size_t nr_digits, bool quot)
{
	size_t n = nr_digits;
	if (likely(quot))
		n += 2;
	if (unlikely((kkt_tx_len + n) > sizeof(kkt_tx)))
		return on_wr_overflow();
	if (likely(quot))
		write_quot();
	for (int i = nr_digits - 1; i >= 0; i--){
		if (v == 0)
			kkt_tx[kkt_tx_len + i] = 0x30;
		else{
			kkt_tx[kkt_tx_len + i] = v % 10 + 0x30;
			v /= 10;
		}
	}
	kkt_tx_len += nr_digits;
	if (likely(quot))
		write_quot();
	return true;
}

static bool write_ip(uint32_t ip, bool quot)
{
	size_t n = (ip == 0) ? 0 : 15;
	if (likely(quot))
		n += 2;
	if (unlikely((kkt_tx_len + n) > sizeof(kkt_tx)))
		return on_wr_overflow();
	if (quot)
		write_quot();
	if (ip != 0){
		for (int i = 0; i < 32; i += 8){
			write_uint((ip >> i) & 0xff, 3, false);
			if (i < 24)
				kkt_tx[kkt_tx_len++] = '.';
		}
	}
	if (likely(quot))
		write_quot();
	return true;
}

static bool write_port(uint16_t port, bool quot)
{
	bool ret = true;
	if (port == 0){
		if (unlikely((kkt_tx_len + 2) > sizeof(kkt_tx)))
			ret = on_wr_overflow();
		else{
			write_quot();
			write_quot();
		}
	}else
		ret = write_uint(port, 5, quot);
	return ret;
}

static bool write_str(const char *str, bool quot)
{
	size_t slen = (str == NULL) ? 0 : strlen(str), len = slen;
	if (quot)
		len += 2;
	if (unlikely((kkt_tx_len + len) > sizeof(kkt_tx)))
		return on_wr_overflow();
	if (likely(quot))
		write_quot();
	if (likely(slen > 0)){
		memcpy(kkt_tx + kkt_tx_len, str, slen);
		kkt_tx_len += slen;
	}
	if (likely(quot))
		write_quot();
	return true;
}

static bool prepare_cmd(uint8_t prefix, uint8_t cmd)
{
	bool ret = false;
	kkt_reset_tx();
	if (prefix == KKT_NUL)
		ret = write_cmd(cmd);
	else
		ret = write_cmd2(prefix, cmd);
	if (ret){
		tx_cmd = cmd;
		tx_prefix = prefix;
	}
	return ret;
}

static uint32_t get_timeout(uint8_t prefix, uint8_t cmd)
{
	uint32_t ret = KKT_DEF_TIMEOUT;
	switch (prefix){
		case KKT_SRV:
			switch (cmd){
				case KKT_SRV_FDO_IFACE:
					ret = KKT_FDO_IFACE_TIMEOUT;
					break;
				case KKT_SRV_FDO_ADDR:
					ret = KKT_FDO_ADDR_TIMEOUT;
					break;
				case KKT_SRV_FDO_DATA:
					ret = KKT_FDO_DATA_TIMEOUT;
					break;
				case KKT_SRV_PRINT_LAST:
				case KKT_SRV_END_DOC:
					ret = KKT_FR_PRINT_TIMEOUT;
					break;
			}
			break;
		case KKT_FS:
			switch (cmd){
				case KKT_FS_STATUS:
					ret = KKT_FR_STATUS_TIMEOUT;
					break;
				case KKT_FS_RESET:
					ret = KKT_FR_RESET_TIMEOUT;
					break;
			}
			break;
	}
	return ret;
}

static bool do_transaction(uint8_t prefix, uint8_t cmd, void *param)
{
	bool ret = true;
	parser_t parser = get_parser(prefix, cmd);
	uint32_t timeout = get_timeout(prefix, cmd);
	struct timeb t0;
	ftime(&t0);
	if (kkt_tx_len > 0){
		ssize_t rc = kkt_io_write(&timeout);
		if (rc != kkt_tx_len)
			ret = kkt_on_com_error(timeout);
	}
	if (ret){
		begin_rx();
		while (ret && (kkt_rx_len < kkt_rx_exp_len)){
			size_t len = kkt_rx_exp_len - kkt_rx_len;
			if ((kkt_rx_len + len) > sizeof(kkt_rx)){
				kkt_status = KKT_STATUS_RESP_FMT_ERROR;
				ret = false;
			}else{
				ssize_t rc = kkt_io_read(len, &timeout);
				if (rc == len){
					kkt_rx_len += rc;
					if (!parser(param)){
						kkt_status = KKT_STATUS_RESP_FMT_ERROR;
						ret = false;
					}
				}else
					ret = kkt_on_com_error(timeout);
			}
		}
	}else
		kkt_rx_len = 0;		/* kkt_reset_rx() обнуляет kkt_status */
	klog_write_rec(hklog, &t0, kkt_tx, kkt_tx_len, kkt_status, kkt_rx, kkt_rx_len);
	return ret;
}

static bool do_cmd(uint8_t prefix, uint8_t cmd, void *param)
{
	bool ret = false;
	if (prepare_cmd(prefix, cmd) && kkt_open_dev_if_need()){
		ret = do_transaction(prefix, cmd, param);
		kkt_close_dev();
	}
	return ret;
}

/* Установить интерфейс взаимодействия с ОФД */
uint8_t kkt_set_fdo_iface(int fdo_iface)
{
	if (prepare_cmd(KKT_SRV, KKT_SRV_FDO_IFACE) && write_byte(fdo_iface + 0x30) &&
			kkt_open_dev_if_need()){
		do_transaction(KKT_SRV, KKT_SRV_FDO_IFACE, NULL);
		kkt_close_dev();
	}
	return kkt_status;
}

/* Установить адрес ОФД */
uint8_t kkt_set_fdo_addr(uint32_t fdo_ip, uint16_t fdo_port)
{
	if (prepare_cmd(KKT_SRV, KKT_SRV_FDO_ADDR) && write_ip(fdo_ip, true) &&
			write_port(fdo_port, true) && kkt_open_dev_if_need()){
		do_transaction(KKT_SRV, KKT_SRV_FDO_ADDR, NULL);
		kkt_close_dev();
	}
	return kkt_status;
}

static void clr_var_data(uint8_t *data, size_t *data_len)
{
	if (data_len != NULL){
		if ((*data_len > 0) && (data != NULL))
			memset(data, 0, *data_len);
		*data_len = 0;
	}
}

static void set_var_data(uint8_t *data, size_t *data_len, const struct kkt_var_data *var_data)
{
	assert(var_data != NULL);
	if (data_len != NULL){
		size_t len = var_data->len;
		if (len > *data_len)
			len = *data_len;
		if ((len > 0) && (data != NULL))
			memcpy(data, var_data->data, len);
		*data_len = var_data->len;
	}
}

/* Получить инструкцию по обмену с ОФД */
uint8_t kkt_get_fdo_cmd(uint8_t prev_op, uint16_t prev_op_status, uint8_t *cmd,
		uint8_t *data, size_t *data_len)
{
	assert(cmd != NULL);
	bool ok = false, vset = false;
	char txt[7];
	if (prev_op == UINT8_MAX)
		snprintf(txt, sizeof(txt), "%s", "-:----");
	else
		snprintf(txt, sizeof(txt), "%c:%.4hX", prev_op, prev_op_status);
	if (prepare_cmd(KKT_SRV, KKT_SRV_FDO_REQ) && write_str(txt, false) &&
			kkt_open_dev_if_need()){
		struct kkt_fdo_cmd fdo_cmd;
		if (do_transaction(KKT_SRV, KKT_SRV_FDO_REQ, &fdo_cmd)){
			*cmd = fdo_cmd.cmd;
			set_var_data(data, data_len, &fdo_cmd.data);
			ok = vset = true;
		}
		kkt_close_dev();
	}
	if (!vset)
		clr_var_data(data, data_len);
	return ok ? KKT_STATUS_OK : kkt_status;
}

/* Передать в ККТ данные, полученные от ОФД */
uint8_t kkt_send_fdo_data(const uint8_t *data, size_t data_len)
{
	assert(data != NULL);
	assert(data_len > 0);
	if (prepare_cmd(KKT_SRV, KKT_SRV_FDO_DATA) && write_word(data_len) &&
			write_data(data, data_len) && kkt_open_dev_if_need()){
		do_transaction(KKT_SRV, KKT_SRV_FDO_DATA, NULL);
		kkt_close_dev();
	}
	return kkt_status;
}

/* Прочитать показания часов реального времени */
uint8_t kkt_get_rtc(struct kkt_rtc_data *rtc)
{
	assert(rtc != NULL);
	memset(rtc, 0, sizeof(*rtc));
	do_cmd(KKT_SRV, KKT_SRV_RTC_GET, rtc);
	return kkt_status;
}

/* Установить часы реального времени */
uint8_t kkt_set_rtc(time_t t)
{
	struct tm *tm = localtime(&t);
	if (prepare_cmd(KKT_SRV, KKT_SRV_RTC_SET) && write_bcd(tm->tm_year + 1900, 2) &&
			write_bcd(tm->tm_mon + 1, 1) && write_bcd(tm->tm_mday, 1) &&
			write_bcd(tm->tm_hour, 1) && write_bcd(tm->tm_min, 1) &&
			write_bcd(tm->tm_sec, 1) && kkt_open_dev_if_need()){
		do_transaction(KKT_SRV, KKT_SRV_RTC_SET, NULL);
		kkt_close_dev();
	}
	return kkt_status;
}

/* Получить информацию о последнем сформированном и последнем отпечатанном документах */
uint8_t kkt_get_last_doc_info(struct kkt_last_doc_info *ldi, uint8_t *err_info,
	size_t *err_info_len)
{
	assert(ldi != NULL);
	bool vset = false;
	struct last_doc_info_arg arg;
	if (prepare_cmd(KKT_SRV, KKT_SRV_LAST_DOC_INFO) && kkt_open_dev_if_need()){
		if (do_transaction(KKT_SRV, KKT_SRV_LAST_DOC_INFO, &arg)){
			*ldi = arg.ldi;
			set_var_data(err_info, err_info_len, &arg.err_info);
			vset = true;
		}
		kkt_close_dev();
	}
	if (!vset)
		clr_var_data(err_info, err_info_len);
	return kkt_status;
}

/* Напечатать последний сформированный документ */
uint8_t kkt_print_last_doc(uint16_t doc_type, const uint8_t *tmpl, size_t tmpl_len,
	struct kkt_last_printed_info *lpi, uint8_t *err_info, size_t *err_info_len)
{
	assert(tmpl != NULL);
	assert(lpi != NULL);
	bool vset = false;
	struct last_printed_info_arg arg;
	if (prepare_cmd(KKT_SRV, KKT_SRV_PRINT_LAST) && write_word(doc_type) &&
			write_word(tmpl_len) &&
			((tmpl_len == 0) || write_data(tmpl, tmpl_len)) && kkt_open_dev_if_need()){
		if (do_transaction(KKT_SRV, KKT_SRV_PRINT_LAST, &arg)){
			*lpi = arg.lpi;
			set_var_data(err_info, err_info_len, &arg.err_info);
			vset = true;
		}
		kkt_close_dev();
	}
	if (!vset)
		clr_var_data(err_info, err_info_len);
	return kkt_status;
}

/* Начать формирование документа */
uint8_t kkt_begin_doc(uint16_t doc_type, uint8_t *err_info, size_t *err_info_len)
{
	bool vset = false;
	struct kkt_var_data arg;
	if (prepare_cmd(KKT_SRV, KKT_SRV_BEGIN_DOC) && write_word(doc_type) &&
			kkt_open_dev_if_need()){
		if (do_transaction(KKT_SRV, KKT_SRV_BEGIN_DOC, &arg)){
			set_var_data(err_info, err_info_len, &arg);
			vset = true;
		}
		kkt_close_dev();
	}
	if (!vset)
		clr_var_data(err_info, err_info_len);
	return kkt_status;
}

/* Передать данные документа */
uint8_t kkt_send_doc_data(const uint8_t *data, size_t len, uint8_t *err_info,
	size_t *err_info_len)
{
	assert(data != NULL);
	assert(len > 0);
	bool vset = false;
	struct kkt_var_data arg;
	if (prepare_cmd(KKT_SRV, KKT_SRV_SEND_DOC) && write_word(len) &&
			write_data(data, len) && kkt_open_dev_if_need()){
		if (do_transaction(KKT_SRV, KKT_SRV_SEND_DOC, &arg)){
			set_var_data(err_info, err_info_len, &arg);
			vset = true;
		}
		kkt_close_dev();
	}
	if (!vset)
		clr_var_data(err_info, err_info_len);
	return kkt_status;
}

/* Завершить формирование документа */
uint8_t kkt_end_doc(uint16_t doc_type, const uint8_t *tmpl, size_t tmpl_len,
	struct kkt_doc_info *di, uint8_t *err_info, size_t *err_info_len)
{
	assert(tmpl != NULL);
	assert(tmpl_len > 0);
	assert(di != NULL);
	bool vset = false;
	struct doc_info_arg arg;
	if (prepare_cmd(KKT_SRV, KKT_SRV_END_DOC) && write_word(tmpl_len) &&
			write_data(tmpl, tmpl_len) && kkt_open_dev_if_need()){
		if (do_transaction(KKT_SRV, KKT_SRV_END_DOC, &arg)){
			set_var_data(err_info, err_info_len, &arg.err_info);
			vset = true;
		}
		kkt_close_dev();
	}
	if (!vset)
		clr_var_data(err_info, err_info_len);
	return kkt_status;
}

/* Настроить сетевой интерфейс ККТ */
uint8_t kkt_set_eth_cfg(uint32_t ip, uint32_t netmask, uint32_t gw)
{
	if (prepare_cmd(KKT_SRV, KKT_SRV_NET_SETTINGS) && write_ip(ip, true) &&
			write_ip(netmask, true) && write_ip(gw, true) &&
			kkt_open_dev_if_need()){
		do_transaction(KKT_SRV, KKT_SRV_NET_SETTINGS, NULL);
		kkt_close_dev();
	}
	return kkt_status;
}

/* Настроить GPRS в ККТ */
uint8_t kkt_set_gprs_cfg(const char *apn, const char *user, const char *password)
{
	if (prepare_cmd(KKT_SRV, KKT_SRV_GPRS_SETTINGS) && write_str(apn, true) &&
			write_str(user, true) && write_str(password, true) &&
			kkt_open_dev_if_need()){
		do_transaction(KKT_SRV, KKT_SRV_GPRS_SETTINGS, NULL);
		kkt_close_dev();
	}
	return kkt_status;
}

/* Настроить ККТ в соответствии с конфигурацией терминала */
uint8_t kkt_set_cfg(void)
{
	uint8_t ret = KKT_STATUS_OK;
	kkt_begin_batch_mode();
	do {
		ret = kkt_set_fdo_iface(cfg.fdo_iface);
		printf("%s: kkt_set_fdo_iface: 0x%.2hhx\n", __func__, ret);
		if (ret != FDO_IFACE_STATUS_OK)
			break;
		ret = kkt_set_fdo_addr(cfg.fdo_ip, cfg.fdo_port);
		printf("%s: kkt_set_fdo_addr: 0x%.2hhx\n", __func__, ret);
		if (ret != FDO_ADDR_STATUS_OK)
			break;
		ret = kkt_set_eth_cfg(cfg.kkt_ip, cfg.kkt_netmask, cfg.kkt_gw);
		printf("%s: kkt_set_eth_cfg: 0x%.2hhx\n", __func__, ret);
		if (ret != NET_SETTINGS_STATUS_OK)
			break;
		ret = kkt_set_gprs_cfg(cfg.kkt_gprs_apn, cfg.kkt_gprs_user, cfg.kkt_gprs_passwd);
		printf("%s: kkt_set_gprs_cfg: 0x%.2hhx\n", __func__, ret);
	} while (false);
	kkt_end_batch_mode();
	return ret;
}


/* Получить статус ФН */
uint8_t kkt_get_fs_status(struct kkt_fs_status *fs_status)
{
	assert(fs_status != NULL);
	memset(fs_status, 0, sizeof(*fs_status));
	do_cmd(KKT_FS, KKT_FS_STATUS, fs_status);
	set_fs_nr((kkt_status == KKT_STATUS_OK) ? fs_status->nr : NULL);
	return kkt_status;
}

/* Получить номер ФН */
uint8_t kkt_get_fs_nr(char *fs_nr)
{
	assert(fs_nr != NULL);
	memset(fs_nr, 0, KKT_FS_NR_LEN);
	do_cmd(KKT_FS, KKT_FS_NR, fs_nr);
	set_fs_nr(kkt_status == KKT_STATUS_OK ? fs_nr : NULL);
	return kkt_status;
}

/* Получить срок действия ФН */
uint8_t kkt_get_fs_lifetime(struct kkt_fs_lifetime *lt)
{
	assert(lt != NULL);
	memset(lt, 0, sizeof(*lt));
	do_cmd(KKT_FS, KKT_FS_LIFETIME, lt);
	return kkt_status;
}

/* Прочитать версию ФН */
uint8_t kkt_get_fs_version(struct kkt_fs_version *ver)
{
	assert(ver != NULL);
	memset(ver, 0, sizeof(*ver));
	do_cmd(KKT_FS, KKT_FS_VERSION, ver);
	return kkt_status;
}

/* Прочитать информацию о последней ошибке ФН */
uint8_t kkt_get_fs_last_error(uint8_t *err_info, size_t *err_info_len)
{
	assert(err_info != NULL);
	assert(err_info_len != NULL);
	struct kkt_var_data data;
	if (do_cmd(KKT_FS, KKT_FS_LAST_ERROR, &data))
		set_var_data(err_info, err_info_len, &data);
	else
		clr_var_data(err_info, err_info_len);
	return kkt_status;
}

/* Получить информацию о текущей смене */
uint8_t kkt_get_fs_shift_state(struct kkt_fs_shift_state *ss)
{
	assert(ss != NULL);
	memset(ss, 0, sizeof(*ss));
	do_cmd(KKT_FS, KKT_FS_SHIFT_PARAMS, ss);
	return kkt_status;
}

/* Получить статус информационного обмена с ОФД */
uint8_t kkt_get_fs_transmission_state(struct kkt_fs_transmission_state *ts)
{
	assert(ts != NULL);
	memset(ts, 0, sizeof(*ts));
	do_cmd(KKT_FS, KKT_FS_TRANSMISSION_STATUS, ts);
	return kkt_status;
}

/* Найти фискальный документ по номеру */
uint8_t kkt_find_fs_doc(uint32_t nr, bool need_print,
	struct kkt_fs_find_doc_info *fdi, uint8_t *data, size_t *data_len)
{
	assert(fdi != NULL);
	assert(data != NULL);
	assert(data_len != NULL);
	bool vset = false;
	memset(fdi, 0, sizeof(*fdi));
	if (prepare_cmd(KKT_FS, KKT_FS_FIND_DOC) && write_dword(nr) &&
			write_byte(need_print) && kkt_open_dev_if_need()){
		struct find_doc_info_arg arg;
		if (do_transaction(KKT_FS, KKT_FS_FIND_DOC, &arg)){
			*fdi = arg.fdi;
			set_var_data(data, data_len, &arg.data);
			vset = true;
		}
		kkt_close_dev();
	}
	if (!vset)
		clr_var_data(data, data_len);
	return kkt_status;
}

/* Найти подтверждение ОФД по номеру */
uint8_t kkt_find_fdo_ack(uint32_t nr, bool need_print, struct kkt_fs_fdo_ack *fdo_ack)
{
	assert(fdo_ack != NULL);
	memset(fdo_ack, 0, sizeof(*fdo_ack));
	if (prepare_cmd(KKT_FS, KKT_FS_FIND_FDO_ACK) && write_dword(nr) &&
			write_byte(need_print) && kkt_open_dev_if_need()){
		do_transaction(KKT_FS, KKT_FS_FIND_FDO_ACK, fdo_ack);
		kkt_close_dev();
	}
	return kkt_status;
}

/* Получить количество документов, на которые нет квитанции ОФД */
uint8_t kkt_get_unconfirmed_docs_nr(uint32_t *nr_docs)
{
	assert(nr_docs != NULL);
	*nr_docs = 0;
	do_cmd(KKT_FS, KKT_FS_UNCONFIRMED_DOC_CNT, nr_docs);
	return kkt_status;
}

/* Получить данные последней регистрции */
uint8_t kkt_get_last_reg_data(uint8_t *data, size_t *data_len)
{
	assert(data != NULL);
	assert(data_len != NULL);
	struct kkt_var_data arg;
	if (do_cmd(KKT_FS, KKT_FS_LAST_REG_DATA, &arg))
		set_var_data(data, data_len, &arg);
	else
		clr_var_data(data, data_len);
	return kkt_status;
}
