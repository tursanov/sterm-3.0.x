/*
 * ¥ç âì â¥ªãé¨å ­ áâà®¥ª ” ­  ç¥ª®¢®© «¥­â¥. ‚ˆŒ€ˆ…: ¨á¯®«ì§ã¥âáï ª®¤¨à®¢ª  CP866.
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
		snprintf(nr, sizeof(nr), "… “‘’€‚‹…");
	return nr;
}

static const char *flow_ctl_str(uint8_t flow_ctl)
{
	static char txt[3];
	const char *ret = NULL;
	switch (flow_ctl + 0x30){
		case FLOW_CTL_NONE:
			ret = "…’";
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
			ret = "…’";
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
			ret = "€‘’‰Š€";
			break;
		case FS_LIFESTATE_FREADY:
			ret = "ƒ’‚ Š ”ˆ‘Š€‹ˆ‡€–ˆˆ";
			break;
		case FS_LIFESTATE_FMODE:
			ret = "”ˆ‘Š€‹œ›‰ …†ˆŒ";
			break;
		case FS_LIFESTATE_POST_FMODE:
			ret = "‘’”ˆ‘Š€‹œ›‰ …†ˆŒ";
			break;
		case FS_LIFESTATE_ARCHIVE_READ:
			ret = "—’…ˆ… ˆ‡ €•ˆ‚€ ”";
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
			ret = "…’";
			break;
		case FS_CURDOC_REG_REPORT:
			ret = "’—…’  …ƒ.";
			break;
		case FS_CURDOC_SHIFT_OPEN_REPORT:
			ret = "’—…’  ’Š. ‘Œ.";
			break;
		case FS_CURDOC_CHEQUE:
			ret = "Š€‘‘‚›‰ —…Š";
			break;
		case FS_CURDOC_SHIFT_CLOSE_REPORT:
			ret = "’—.  ‡€Š. ‘Œ.";
			break;
		case FS_CURDOC_FMODE_CLOSE_REPORT:
			ret = "’—.  ‡€Š. ”";
			break;
		case FS_CURDOC_BSO:
			ret = "‘";
			break;
		case FS_CURDOC_REG_PARAMS_REPORT_ON_FS_CHANGE:
			ret = "’—…’  ………ƒ. (‡€Œ. ”)";
			break;
		case FS_CURDOC_REG_PARAMS_REPORT:
			ret = "’—…’  ………ƒ.";
			break;
		case FS_CURDOC_CORRECTION_CHEQUE:
			ret = "—…Š Š…Š–ˆˆ";
			break;
		case FS_CURDOC_CORRECTION_BSO:
			ret = "‘ Š…Š–ˆˆ";
			break;
		case FS_CURDOC_CURRENT_PAY_REPORT:
			ret = "’—…’  €‘—.";
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
			ret = "…’";
			break;
		case FS_ALERT_CC_REPLACE_URGENT:
			ret = "‘—€Ÿ ‡€Œ…€ Š‘";
			break;
		case FS_ALERT_CC_EXHAUST:
			ret = "ˆ‘—…€ˆ… …‘“‘€ Š‘";
			break;
		case FS_ALERT_MEMORY_FULL:
			ret = "……‹…ˆ… €ŒŸ’ˆ ”";
			break;
		case FS_ALERT_RESP_TIMEOUT:
			ret = "’€‰Œ€“’ ’‚…’€ ”„";
			break;
		case FS_ALERT_CRITRICAL:
			ret = "Šˆ’ˆ—. ˜. ”";
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
		ret = "…ˆ‡‚…‘’";
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
			ret = "’‹€„—›‰";
			break;
		case 1:
			ret = "‘…ˆ‰›‰";
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
		"€‚ƒ„…ğ†‡ˆ‰Š‹Œ‘’“”•–—˜™š›œŸ"
		" ¡¢£¤¥ñ¦§¨©ª«¬­®¯àáâãäåæçèéêëìíîï";
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
		snprintf(fs_version, sizeof(fs_version), "…ˆ‡‚…‘’");
	cheque_len = snprintf((char *)cheque, CHEQUE_BUF_SIZE,
			"\033\176\006\036\007ŠŠ’ \"‘…Š’-”\"\n\r"
			"\004‡ ¢®¤áª®© ­®¬¥à ŠŠ’:\005%s\n\r"
			"\004‚¥àá¨ï :\005" _s(VERSION_MAJOR) "." _s(VERSION_MINOR) "." _s(VERSION_REVISION) "\n\r"
			"\004®ª § ­¨ï RTC:\005%s\n\r"
			"\004‘ª®à®áâì COM-¯®àâ :\005115200 „\n\r"
			"\004“¯à ¢«¥­¨¥ ¯®â®ª®¬:\005%s\n\r"
			"\004‘¨£­ «¨§ æ¨ï á¡à®á  ª ­ « :\005%s\n\r"
			"\004âà¥§ª  ç¥ª®¢®© «¥­âë:\005%s\n\r\n\r"

			"\006”¨áª «ì­ë© ­ ª®¯¨â¥«ì\n\r"
			"\004‡ ¢®¤áª®© ­®¬¥à ”:\005%s\n\r"
			"\004‚¥àá¨ï:\005%s\n\r"
			"\004’¨¯:\005%s\n\r"
			"\004” §  ¦¨§­¨:\005%s\n\r"
			"\004’¥ªãè¨© ¤®ªã¬¥­â:\005%s\n\r"
			"\004‘¬¥­ :\005%s\n\r"
			"\004à¥¤ã¯à¥¦¤¥­¨ï:\005%s\n\r"
			"\004®á«¥¤­¨© ¤®ªã¬¥­â:\005%s %s\n\r"
			"\004\374 ¯®á«¥¤­¥£® ¤®ªã¬¥­â :\005%lu\n\r"
			"\004‘à®ª ¤¥©áâ¢¨ï:\005%s\n\r"
			"\004‚ë¯®«­¥­® à¥£¨áâà æ¨©:\005%u\n\r"
			"\004áâ «®áì à¥£¨áâà æ¨©:\005%u\n\r\n\r"

			"\006 áâà®©ª¨ Ethernet\n\r"
			"\004MAC- ¤à¥á:\005%.2hhX:%.2hhX:%.2hhX:%.2hhX:%.2hhX:%.2hhX\n\r"
			"\004IP- ¤à¥á:\005%lu.%lu.%lu.%lu\n\r"
			"\004Œ áª  ¯®¤á¥â¨:\005%lu.%lu.%lu.%lu\n\r"
			"\004IP- ¤à¥á è«î§ :\005%lu.%lu.%lu.%lu\n\r\n\r"

			"\006 áâà®©ª¨ GPRS\n\r"
			"\004ˆ¬ï â®çª¨ ¤®áâã¯ :\005[%s]"
			"\004ˆ¬ï ¯®«ì§®¢ â¥«ï:\005[%s]"
			"\004 à®«ì:\005[%s]\n\r\n\r"

			"\006”„\n\r"
			"\004ˆ­â¥àä¥©á á ”„:\005%s\n\r"
			"\004IP- ¤à¥á ”„:\005%lu.%lu.%lu.%lu"
			"\004TCP-¯®àâ ”„:\005%hu\n\r\n\r"

			"\006˜à¨äâë:\n\r\n\r"
			"\006\03612 CPI\n\r\034%s\n\r\n\r"
			"\006\03615 CPI\n\r\031%s\n\r\n\r"
			"\006\03620 CPI\n\r%s\n\r\n\r"

			"\006Š®­â ªâ­ ï ¨­ä®à¬ æ¨ï\n\r"
			"\004à®¨§¢®¤¨â¥«ì:\005€ – \"‘¯¥ªâà\"\n\r"
			"\004€¤à¥á:\005®áá¨ï, £.‘ ¬ à , ‡ ¢®¤áª®¥ è®áá¥, 1\n\r"
			"\004â¥«.\005+7 (846) 992-67-46\n\r"
			"\005+7 (846) 955-38-24\n\r"
			"\004â¥«./ä ªá:\005+7 (846) 992-07-49\n\r"
			"\004WWW:\005http://spc.com.ru\n\r"
			"\004eMail:\005spektr@mail.radiant.ru\n\r"
			"\005express@spc.com.ru [â¥å­¨ç¥áª¨¥ ¢®¯à®áë]\n\r\n\r"
			"\033\0326;;017http://spc.com.ru"
			"\014",
		fr_nr_str(), rtc_date_time_str(), flow_ctl_str(fr_cfg->flow_ctl), rst_type_str(fr_cfg->rst_type),
		fr_cfg->use_cutter ? "…‘’œ" : "…’",

		fs_nr_str(fs_status_ok ? &fs_status : NULL),
		fs_version,
		fs_version_ok ? fs_type_str(fs_type) : "…’",
		fs_status_ok ? fs_lifestate_str(fs_status.life_state) : "…’",
		fs_status_ok ? fs_doc_type_str(fs_status.current_doc) : "…’",
		fs_status_ok ? (fs_status.shift_state ? "’Š›’€" : "‡€Š›’€") : "…’",
		fs_status_ok ? fs_alert_str(fs_status.alert_flags) : "…’",
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
