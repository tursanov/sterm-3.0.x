/*
 * ����� ⥪��� ����஥� �� �� 祪���� ����. ��������: �ᯮ������ ����஢�� CP866.
 * (c) gsr, 2018.
 */

#include "hw/fs.h"
#include "hw/rtc.h"
#include "bitmap.h"
#include "cfg.h"
#include "core.h"
#include "printer.h"
#include "version.h"

static const char *fr_nr_str(void)
{
	static char nr[FR_NR_LEN + 1];
	if (fr_nr_valid)
		snprintf(nr, sizeof(nr), "%.*s", FR_NR_LEN, fr_params->fr_nr);
	else
		snprintf(nr, sizeof(nr), "�� ����������");
	return nr;
}

static const char *flow_ctl_str(uint8_t flow_ctl)
{
	static char txt[3];
	const char *ret = NULL;
	switch (flow_ctl + 0x30){
		case FLOW_CTL_NONE:
			ret = "���";
			break;
		case FLOW_CTL_XON_XOFF:
			ret = "XON/XOFF";
			break;
		case FLOW_CTL_RTS_CTS:
			ret = "RTS/CTS";
			break;
		case FLOW_CTL_DTR_DSR:
			ret = "DTR/DSR";
			break;
		default:
			snprintf(txt, sizeof(txt), "%.2hhXh", flow_ctl);
			ret = txt;
	}
	return ret;
}

static const char *rst_type_str(uint8_t rst_type)
{
	static char txt[3];
	const char *ret = NULL;
	switch (rst_type + 0x30){
		case RST_TYPE_NONE:
			ret = "���";
			break;
		case RST_TYPE_BREAK_STATE:
			ret = "Break State";
			break;
		case RST_TYPE_RTS_CTS:
			ret = "RTS/CTS";
			break;
		case RST_TYPE_DTR_DSR:
			ret = "DTR/DSR";
			break;
		default:
			snprintf(txt, sizeof(txt), "%.2hhXh", rst_type);
			ret = txt;
	}
	return ret;
}

static const char *fdo_iface_str(uint8_t fdo_iface)
{
	static char txt[3];
	const char *ret = NULL;
	switch (fdo_iface + 0x30){
		case FDO_IFACE_USB:
		case FDO_IFACE_OTHER:
			ret = "USB";
			break;
		case FDO_IFACE_ETHER:
			ret = "ETHERNET";
			break;
		case FDO_IFACE_GPRS:
			ret = "GPRS";
			break;
		default:
			snprintf(txt, sizeof(txt), "%.2hhXh", fdo_iface);
			ret = txt;
	}
	return ret;
}

static const char *fs_lifestate_str(uint8_t lifestate)
{
	static char txt[4];
	const char *ret = NULL;
	switch (lifestate){
		case FS_LIFESTATE_NONE:
			ret = "���������";
			break;
		case FS_LIFESTATE_FREADY:
			ret = "����� � ������������";
			break;
		case FS_LIFESTATE_FMODE:
			ret = "���������� �����";
			break;
		case FS_LIFESTATE_POST_FMODE:
			ret = "�������������� �����";
			break;
		case FS_LIFESTATE_ARCHIVE_READ:
			ret = "������ �� ������ ��";
			break;
		default:
			snprintf(txt, sizeof(txt), "%hhu", lifestate);
			ret = txt;
	}
	return ret;
}

static const char *fs_doc_type_str(uint8_t doc_type)
{
	static char txt[4];
	const char *ret = NULL;
	switch (doc_type){
		case FS_CURDOC_NOT_OPEN:
			ret = "���";
			break;
		case FS_CURDOC_REG_REPORT:
			ret = "����� � ���.";
			break;
		case FS_CURDOC_SHIFT_OPEN_REPORT:
			ret = "����� �� ����. ��.";
			break;
		case FS_CURDOC_CHEQUE:
			ret = "�������� ���";
			break;
		case FS_CURDOC_SHIFT_CLOSE_REPORT:
			ret = "���. � ����. ��.";
			break;
		case FS_CURDOC_FMODE_CLOSE_REPORT:
			ret = "���. � ����. ��";
			break;
		case FS_CURDOC_BSO:
			ret = "���";
			break;
		case FS_CURDOC_REG_PARAMS_REPORT_ON_FS_CHANGE:
			ret = "����� � �������. (���. ��)";
			break;
		case FS_CURDOC_REG_PARAMS_REPORT:
			ret = "����� � �������.";
			break;
		case FS_CURDOC_CORRECTION_CHEQUE:
			ret = "��� ���������";
			break;
		case FS_CURDOC_CORRECTION_BSO:
			ret = "��� ���������";
			break;
		case FS_CURDOC_CURRENT_PAY_REPORT:
			ret = "����� � ����.";
			break;
		default:
			snprintf(txt, sizeof(txt), "%hhu", doc_type);
			ret = txt;
	}
	return ret;
}

static const char *fs_alert_str(uint8_t alert)
{
	static char txt[3];
	const char *ret = NULL;
	switch (alert){
		case 0:
			ret = "���";
			break;
		case FS_ALERT_CC_REPLACE_URGENT:
			ret = "������� ������ ��";
			break;
		case FS_ALERT_CC_EXHAUST:
			ret = "���������� ������� ��";
			break;
		case FS_ALERT_MEMORY_FULL:
			ret = "������������ ������ ��";
			break;
		case FS_ALERT_RESP_TIMEOUT:
			ret = "������� ������ ���";
			break;
		case FS_ALERT_CRITRICAL:
			ret = "������. ��. ��";
			break;
		default:
			snprintf(txt, sizeof(txt), "%.2hhXh", alert);
			ret = txt;
	}
	return ret;
}

static const char *fs_date_str(const fs_date_t *date)
{
	static char txt[10][11];
	static int txt_idx = 0;
	snprintf(txt[txt_idx], sizeof(txt[txt_idx]), "%.2hhu.%.2hhu.%u", date->day, date->month, date->year + 2000);
	const char *ret = txt[txt_idx++];
	txt_idx %= ASIZE(txt);
	return ret;
}

static const char *fs_time_str(const fs_time_t *time)
{
	static char txt[10][9];
	static int txt_idx = 0;
	snprintf(txt[txt_idx], sizeof(txt[txt_idx]), "%.2hhu:%.2hhu", time->hour, time->minute);
	const char *ret = txt[txt_idx++];
	txt_idx %= ASIZE(txt);
	return ret;
}

static const char *fs_nr_str(const fs_status_t *fs_status)
{
	static char fs_nr[FS_SERIAL_NUMBER_LEN + 1];
	const char *ret = fs_nr;
	if (fs_status == NULL)
		ret = "����������";
	else
		snprintf(fs_nr, sizeof(fs_nr), "%.*s", FS_SERIAL_NUMBER_LEN, fs_status->serial_number);
	return ret;
}

static const char *fs_type_str(uint8_t type)
{
	static char txt[3];
	const char *ret = NULL;
	switch (type){
		case 0:
			ret = "����������";
			break;
		case 1:
			ret = "��������";
			break;
		default:
			snprintf(txt, sizeof(txt), "%.2hhXh", type);
			ret = txt;
	}
	return ret;
}

void print_fr_params(void)
{
	static const char fnt_chars[] __attribute__((__section__(".rodata"))) =
		"!\"#\374$%&'()*+,-./0123456789:;<=>?@"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
		"abcdefghijklmnopqrstuvwxyz{|}~"
		"���������������������������������"
		"������񦧨�����������������������";
	fs_status_t fs_status;
	bool fs_status_ok = fs_get_status(&fs_status, 1000) == FS_RET_SUCCESS;
	fs_lifetime_t fs_lifetime;
	bool fs_lifetime_ok = fs_get_lifetime(&fs_lifetime, 1000) == FS_RET_SUCCESS;
	char fs_version[FS_VERSION_LEN + 1];
	uint8_t fs_type = 0;
	bool fs_version_ok = fs_get_version(fs_version, &fs_type, 1000) == FS_RET_SUCCESS;
	if (fs_version_ok)
		fs_version[FS_VERSION_LEN] = 0;
	else
		snprintf(fs_version, sizeof(fs_version), "����������");
	cheque_len = snprintf((char *)cheque, CHEQUE_BUF_SIZE,
			"\033\176\006\036\007��� \"������-�\"\n\r"
			"\004�����᪮� ����� ���:\005%s\n\r"
			"\004����� ��:\005" _s(VERSION_MAJOR) "." _s(VERSION_MINOR) "." _s(VERSION_REVISION) "\n\r"
			"\004��������� RTC:\005%s\n\r"
			"\004������� COM-����:\005115200 ���\n\r"
			"\004��ࠢ����� ��⮪��:\005%s\n\r"
			"\004����������� ��� ������:\005%s\n\r"
			"\004��१�� 祪���� �����:\005%s\n\r\n\r"

			"\006��᪠��� ������⥫�\n\r"
			"\004�����᪮� ����� ��:\005%s\n\r"
			"\004�����:\005%s\n\r"
			"\004���:\005%s\n\r"
			"\004���� �����:\005%s\n\r"
			"\004����訩 ���㬥��:\005%s\n\r"
			"\004�����:\005%s\n\r"
			"\004�।�०�����:\005%s\n\r"
			"\004��᫥���� ���㬥��:\005%s %s\n\r"
			"\004\374 ��᫥����� ���㬥��:\005%lu\n\r"
			"\004�ப ����⢨�:\005%s\n\r"
			"\004�믮����� ॣ����権:\005%u\n\r"
			"\004��⠫��� ॣ����権:\005%u\n\r\n\r"

			"\006����ன�� Ethernet\n\r"
			"\004MAC-����:\005%.2hhX:%.2hhX:%.2hhX:%.2hhX:%.2hhX:%.2hhX\n\r"
			"\004IP-����:\005%lu.%lu.%lu.%lu\n\r"
			"\004��᪠ �����:\005%lu.%lu.%lu.%lu\n\r"
			"\004IP-���� �:\005%lu.%lu.%lu.%lu\n\r\n\r"

			"\006����ன�� GPRS\n\r"
			"\004��� �窨 ����㯠:\005[%s]"
			"\004��� ���짮��⥫�:\005[%s]"
			"\004��஫�:\005[%s]\n\r\n\r"

			"\006���\n\r"
			"\004����䥩� � ���:\005%s\n\r"
			"\004IP-���� ���:\005%lu.%lu.%lu.%lu"
			"\004TCP-���� ���:\005%hu\n\r\n\r"

			"\006�����:\n\r\n\r"
			"\006\03612 CPI\n\r\034%s\n\r\n\r"
			"\006\03615 CPI\n\r\031%s\n\r\n\r"
			"\006\03620 CPI\n\r%s\n\r\n\r"

			"\006���⠪⭠� ���ଠ��\n\r"
			"\004�ந�����⥫�:\005�� ��� \"������\"\n\r"
			"\004����:\005�����, �.�����, �����᪮� ���, 1\n\r"
			"\004⥫.\005+7 (846) 992-67-46\n\r"
			"\005+7 (846) 955-38-24\n\r"
			"\004⥫./䠪�:\005+7 (846) 992-07-49\n\r"
			"\004WWW:\005http://spc.com.ru\n\r"
			"\004eMail:\005spektr@mail.radiant.ru\n\r"
			"\005express@spc.com.ru [�孨�᪨� ������]\n\r\n\r"
			"\033\0326;;017http://spc.com.ru"
			"\014",
		fr_nr_str(), rtc_date_time_str(), flow_ctl_str(fr_cfg->flow_ctl), rst_type_str(fr_cfg->rst_type),
		fr_cfg->use_cutter ? "����" : "���",

		fs_nr_str(fs_status_ok ? &fs_status : NULL),
		fs_version,
		fs_version_ok ? fs_type_str(fs_type) : "���",
		fs_status_ok ? fs_lifestate_str(fs_status.life_state) : "���",
		fs_status_ok ? fs_doc_type_str(fs_status.current_doc) : "���",
		fs_status_ok ? (fs_status.shift_state ? "�������" : "�������") : "���",
		fs_status_ok ? fs_alert_str(fs_status.alert_flags) : "���",
		fs_status_ok ? fs_date_str(&fs_status.date_time.date) : "00.00.0000",
		fs_status_ok ? fs_time_str(&fs_status.date_time.time) : "00:00",
		fs_status_ok ? fs_status.last_doc_number : 0,
		fs_lifetime_ok ? fs_date_str(&fs_lifetime.date) : "00.00.0000",
		fs_lifetime_ok ? fs_lifetime.number_of_registrations : 0,
		fs_lifetime_ok ? fs_lifetime.remaining_number_of_registrations : 0,

		hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3], hw_addr[4], hw_addr[5],
		(fr_cfg->ip >> 24) & 0xff, (fr_cfg->ip >> 16) & 0xff, (fr_cfg->ip >> 8) & 0xff, fr_cfg->ip & 0xff,
		(fr_cfg->netmask >> 24) & 0xff, (fr_cfg->netmask >> 16) & 0xff, (fr_cfg->netmask >> 8) & 0xff, fr_cfg->netmask & 0xff,
		(fr_cfg->gw >> 24) & 0xff, (fr_cfg->gw >> 16) & 0xff, (fr_cfg->gw >> 8) & 0xff, fr_cfg->gw & 0xff,

		gprs_apn, gprs_user, gprs_password,

		fdo_iface_str(fr_cfg->fdo_iface),
		(fr_cfg->fdo_ip >> 24) & 0xff, (fr_cfg->fdo_ip >> 16) & 0xff, (fr_cfg->fdo_ip >> 8) & 0xff, fr_cfg->fdo_ip & 0xff,
		fr_cfg->fdo_port,

		fnt_chars, fnt_chars, fnt_chars
	);
	fr_disable_sending();
	parse_cheque();
	fr_enable_sending();
}
