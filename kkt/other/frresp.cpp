/* Разбор ответов фискального регистратора "СПЕКТР-Ф". (c) gsr, 2018 */

#include "stdafx.h"

#include "fr/resp.h"
#include "txtutil.h"

SIZE_T FrRespBase::rx_len() const
{
	SIZE_T ret = 0;
	switch (_ps){
		case ParseState::Prefix:
			ret = _prefix_len;
			break;
		case ParseState::FixData:
			ret = _fix_len;
			break;
		case ParseState::VarDataLen:
			ret = 2;
			break;
		case ParseState::VarData:
			ret = _var_len;
			break;
	}
	return ret;
}

BOOL FrRespBase::parsePrefix(LPCBYTE data, SIZE_T len)
{
	_ASSERTE(data != NULL);
	_ASSERTE(len > 0);
	BOOL ret = TRUE;
	if (len < _prefix_len)
		ret = FALSE;
	else if (data[0] != FrCmd::CMD_ESC2)
		ret = FALSE;
	else if (_prefix == FrCmd::CMD_NUL){
		if (data[1] != _cmd)
			ret = FALSE;
	}else if ((data[1] != _prefix) || (data[2] != _cmd))
		ret = FALSE;
	if (ret){
		_status = data[_prefix_len - 1];
		if (statusOk() && (_fix_len > 0))
			_ps = ParseState::FixData;
		else if (_var_len > 0)
			_ps = ParseState::VarDataLen;
		else
			_ps = ParseState::End;
	}
	return ret;
}

BOOL FrRespBase::parseFixData(LPCBYTE data, SIZE_T len)
{
	_ASSERTE(data != NULL);
	_ASSERTE(len > 0);
	BOOL ret = TRUE;
	if (len < _fix_len)
		ret = FALSE;
	return ret;
}

BOOL FrRespBase::parseVarDataLen(LPCBYTE data, SIZE_T len)
{
	_ASSERTE(data != NULL);
	_ASSERTE(len > 0);
	BOOL ret = TRUE;
	if (len < 2)
		ret = FALSE;
	else{
		_var_len = *reinterpret_cast<const WORD *>(data);
		if (_var_len > 0)
			_ps = ParseState::VarData;
		else
			_ps = ParseState::End;
	}
	return ret;
}

BOOL FrRespBase::parseVarData(LPCBYTE data, SIZE_T len)
{
	_ASSERTE(data != NULL);
	_ASSERTE(len > 0);
	BOOL ret = TRUE;
	if (len < _var_len)
		ret = FALSE;
	else{
		_var_data = new BYTE[_var_len];
		memcpy(_var_data, data, _var_len);
		_ps = ParseState::End;
	}
	return ret;
}

BOOL FrRespBase::parse(LPCBYTE data, SIZE_T len)
{
	_ASSERTE(data != NULL);
	_ASSERTE(len > 0);
	BOOL ret = FALSE;
	switch (_ps){
		case ParseState::Prefix:
			ret = parsePrefix(data, len);
			break;
		case ParseState::FixData:
			ret = parseFixData(data, len);
			break;
		case ParseState::VarDataLen:
			ret = parseVarDataLen(data, len);
			break;
		case ParseState::VarData:
			ret = parseVarData(data, len);
			break;
	}
	return ret;
}

FrRespBase::~FrRespBase()
{
	if (_var_data != NULL)
		delete [] _var_data;
}


BOOL FrRespGetRtc::checkRtcData(LPCFR_RTC_DATA rtc)
{
	BOOL ret = TRUE;
	static BYTE mdays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if ((rtc->year < 2018) || (rtc->month < 1) || (rtc->month > 12) || (rtc->day < 1))
		ret = FALSE;
	else if ((rtc->month == 2) && ((rtc->year % 4) == 0) && (rtc->day > 29))
		ret = FALSE;
	else if (rtc->day > mdays[rtc->month])
		ret = FALSE;
	else if ((rtc->hour > 23) || (rtc->minute > 59) || (rtc->second > 59))
		ret = FALSE;
	return ret;
}

BOOL FrRespGetRtc::parseFixData(LPCBYTE data, SIZE_T len)
{
	BOOL ret = FrRespBase::parseFixData(data, len);
	if (ret){
		if (is_bcd(data[0]) && is_bcd(data[1]))
			_rtc.year = from_bcd(data[0]) * 100 + from_bcd(data[1]);
		else
			ret = FALSE;
		if (ret && is_bcd(data[2]))
			_rtc.month = from_bcd(data[2]);
		else
			ret = FALSE;
		if (ret && is_bcd(data[3]))
			_rtc.day = from_bcd(data[3]);
		else
			ret = FALSE;
		if (ret && is_bcd(data[4]))
			_rtc.hour = from_bcd(data[4]);
		else
			ret = FALSE;
		if (ret && is_bcd(data[5]))
			_rtc.minute = from_bcd(data[5]);
		else
			ret = FALSE;
		if (ret && is_bcd(data[6]))
			_rtc.second = from_bcd(data[6]);
		else
			ret = FALSE;
		if (ret){
			ret = isRtcValid();
			if (ret)
				_ps = ParseState::End;
		}
	}
	return ret;
}


BOOL FrRespLastDocInfo::parseFixData(LPCBYTE data, SIZE_T len)
{
	BOOL ret = FrRespBase::parseFixData(data, len);
	if (ret){
		_ldi.last_nr = *reinterpret_cast<const DWORD *>(data);
		_ldi.last_type = *reinterpret_cast<const WORD *>(data + 4);
		_ldi.last_printed_nr = *reinterpret_cast<const DWORD *>(data + 6);
		_ldi.last_printed_type = *reinterpret_cast<const WORD *>(data + 10);
		_ps = ParseState::VarDataLen;
	}
	return ret;
}


BOOL FrRespPrintLast::parseFixData(LPCBYTE data, SIZE_T len)
{
	BOOL ret = FrRespBase::parseFixData(data, len);
	if (ret){
		_lpi.nr = *reinterpret_cast<const DWORD *>(data);
		_lpi.sign = *reinterpret_cast<const DWORD *>(data + 4);
		_lpi.type = *reinterpret_cast<const WORD *>(data + 8);
		_ps = ParseState::VarDataLen;
	}
	return ret;
}


BOOL FrRespEndDoc::parseFixData(LPCBYTE data, SIZE_T len)
{
	BOOL ret = FrRespBase::parseFixData(data, len);
	if (ret){
		_di.nr = *reinterpret_cast<const DWORD *>(data);
		_di.sign = *reinterpret_cast<const DWORD *>(data + 4);
		_ps = ParseState::VarDataLen;
	}
	return ret;
}


BOOL FrRespFsStatus::parseFixData(LPCBYTE data, SIZE_T len)
{
	BOOL ret = FrRespBase::parseFixData(data, len);
	if (ret){
		_fs_status.life_state = *data++;
		_fs_status.current_doc = *data++;
		_fs_status.doc_data = *data++;
		_fs_status.shift_state = *data++;
		_fs_status.alert_flags = *data++;
		_fs_status.date_time.date.year = from_bcd(*data++) + 2000;
		_fs_status.date_time.date.month = from_bcd(*data++);
		_fs_status.date_time.date.day = from_bcd(*data++);
		_fs_status.date_time.time.hour = from_bcd(*data++);
		_fs_status.date_time.time.minute = from_bcd(*data++);
		_sntprintf_s(_fs_status.serial_number, ARRAYSIZE(_fs_status.serial_number), _TRUNCATE, _T("%s"),
			ansi2tstr(reinterpret_cast<LPCSTR>(data), FS_SERIAL_NUMBER_LEN).c_str());
		data += FS_SERIAL_NUMBER_LEN;
		_fs_status.last_doc_number = *reinterpret_cast<LPCDWORD>(data);
		_ps = ParseState::End;
	}
	return ret;
}


BOOL FrRespFsNr::parseFixData(LPCBYTE data, SIZE_T len)
{
	BOOL ret = FrRespBase::parseFixData(data, len);
	if (ret){
		_sntprintf_s(_fs_nr, ARRAYSIZE(_fs_nr), _TRUNCATE, _T("%s"),
			ansi2tstr(reinterpret_cast<LPCSTR>(data), FS_NR_LEN).c_str());
		_ps = ParseState::End;
	}
	return ret;
}


BOOL FrRespFsLifetime::parseFixData(LPCBYTE data, SIZE_T len)
{
	BOOL ret = FrRespBase::parseFixData(data, len);
	if (ret){
		_fs_lifetime.dt.date.year = from_bcd(*data++) + 2000;
		_fs_lifetime.dt.date.month = from_bcd(*data++);
		_fs_lifetime.dt.date.day = from_bcd(*data++);
		_fs_lifetime.dt.time.hour = from_bcd(*data++);
		_fs_lifetime.dt.time.minute = from_bcd(*data++);
		_fs_lifetime.reg_remain = *data++;
		_fs_lifetime.reg_complete = *data++;
		_ps = ParseState::End;
	}
	return ret;
}


BOOL FrRespFsVersion::parseFixData(LPCBYTE data, SIZE_T len)
{
	BOOL ret = FrRespBase::parseFixData(data, len);
	if (ret){
		_sntprintf_s(_fs_version.version, ARRAYSIZE(_fs_version.version), _TRUNCATE, _T("%s"),
			ansi2tstr(reinterpret_cast<LPCSTR>(data), FS_VERSION_LEN).c_str());
		_fs_version.fw_type = data[FS_VERSION_LEN];
		_ps = ParseState::End;
	}
	return ret;
}


BOOL FrRespShiftParams::parseFixData(LPCBYTE data, SIZE_T len)
{
	BOOL ret = FrRespBase::parseFixData(data, len);
	if (ret){
		_shift_params.opened = data[0] ? TRUE : FALSE;
		_shift_params.shift_nr = *reinterpret_cast<LPCWORD>(data + 1);
		_shift_params.cheque_nr = *reinterpret_cast<LPCWORD>(data + 3);
		_ps = ParseState::End;
	}
	return ret;
}

BOOL FrRespFdoTransmissionStatus::parseFixData(LPCBYTE data, SIZE_T len)
{
	BOOL ret = FrRespBase::parseFixData(data, len);
	if (ret){
		_ts.transmission_state = data[0];
		_ts.msg_read_started = data[1] != 0;
		_ts.snd_queue_len = *reinterpret_cast<LPCWORD>(data + 2);
		_ts.first_doc_nr = *reinterpret_cast<LPCDWORD>(data + 4);
		_ts.first_doc_dt.date.year = from_bcd(data[8]) + 2000;
		_ts.first_doc_dt.date.month = from_bcd(data[9]);
		_ts.first_doc_dt.date.day = from_bcd(data[10]);
		_ts.first_doc_dt.time.hour = from_bcd(data[11]);
		_ts.first_doc_dt.time.minute = from_bcd(data[12]);
		_ps = ParseState::End;
	}
	return ret;
}


SIZE_T FrRespFindDoc::getDocDataLen(BYTE doc_type)
{
	SIZE_T ret = 0;
	switch (doc_type){
		case FrDocType::Registration:
			ret = sizeof(FR_REGISTER_REPORT);
			break;
		case FrDocType::ReRegistration:
			ret = sizeof(FR_REREGISTER_REPORT);
			break;
		case FrDocType::OpenShift:
		case FrDocType::CloseShift:
			ret = sizeof(FR_SHIFT_REPORT);
			break;
		case FrDocType::CalcReport:
			ret = sizeof(FR_CALC_REPORT);
			break;
		case FrDocType::Cheque:
		case FrDocType::ChequeCorr:
		case FrDocType::BSO:
		case FrDocType::BSOCorr:
			ret = sizeof(FR_CHEQUE_REPORT);
			break;
		case FrDocType::CloseFS:
			ret = sizeof(FR_CLOSE_FS);
			break;
		case FrDocType::OpCommit:
			ret = sizeof(FR_FDO_ACK);
			break;
	}
	return ret;
}

BOOL FrRespFindDoc::parseFixData(LPCBYTE data, SIZE_T len)
{
	BOOL ret = FrRespBase::parseFixData(data, len);
	if (ret){
		_fdi.doc_type = data[0];
		_fdi.fdo_ack = data[1] != 0;
		_var_len = getDocDataLen(_fdi.doc_type);
		if (_var_len > 0)
			_ps = ParseState::VarData;
		else
			_ps = ParseState::End;
	}
	return ret;
}


BOOL FrRespUnconfirmedDocsNr::parseFixData(LPCBYTE data, SIZE_T len)
{
	BOOL ret = FrRespBase::parseFixData(data, len);
	if (ret){
		_nr_unconfirmed = *reinterpret_cast<LPCWORD>(data);
		_ps = ParseState::End;
	}
	return ret;
}


BOOL FrRespFindFdoAck::parseFixData(LPCBYTE data, SIZE_T len)
{
	BOOL ret = FrRespBase::parseFixData(data, len);
	if (ret){
		memcpy(&_fdo_ack, data, sizeof(_fdo_ack));
		_ps = ParseState::End;
	}
	return ret;
}
