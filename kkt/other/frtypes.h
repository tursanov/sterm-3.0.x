/* ���� � ����⠭�� ��� ࠡ��� � ��. (c) gsr, 2018 */

#pragma once

#include "termcore.h"

/* ������� �������� �� ������ � ��� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetFdoCmd(BYTE prev_op, WORD prev_op_status, LPBYTE cmd,
	LPBYTE data, LPDWORD data_len);


/* ��।��� � �� �����, ����祭�� �� ��� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrSendFdoData(LPCBYTE data, DWORD data_len, LPBYTE status);


/* ���ଠ�� �ᮢ ॠ�쭮�� �६��� */
typedef struct _FR_RTC_DATA {
	DWORD year;		/* ��� (���ਬ��, 2019) */
	DWORD month;		/* ����� (1 -- 12) */
	DWORD day;		/* ���� (1 -- 31) */
	DWORD hour;		/* �� (0 -- 23) */
	DWORD minute;		/* ����� (0 -- 59) */
	DWORD second;		/* ᥪ㭤� (0 -- 59) */
} FR_RTC_DATA, FAR *LPFR_RTC_DATA;

typedef const FR_RTC_DATA FAR *LPCFR_RTC_DATA;


/* ���ଠ�� � ��᫥���� ��ନ஢����� � ��᫥���� �⯥�⠭��� �᪠���� ���㬥��� */
typedef struct _FR_LAST_DOC_INFO {
	DWORD last_nr;
	WORD last_type;
	DWORD last_printed_nr;
	WORD last_printed_type;
} FR_LAST_DOC_INFO, FAR *LPFR_LAST_DOC_INFO;

typedef const FR_LAST_DOC_INFO FAR *LPCFR_LAST_DOC_INFO;

/* ������� ����� � ��᫥���� ��ନ஢����� � ��᫥���� �⯥�⠭��� �᪠���� ���㬥��� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetLastDocInfo(LPBYTE status, LPFR_LAST_DOC_INFO ldi,
		LPBYTE err_info, LPDWORD err_info_len);


/* ���ଠ�� � ��᫥��� �⯥�⠭��� ���㬥�� */
typedef struct _FR_LAST_PRINTED_INFO {
	DWORD nr;
	DWORD sign;
	WORD type;
} FR_LAST_PRINTED_INFO, FAR *LPFR_LAST_PRINTED_INFO;

typedef const FR_LAST_PRINTED_INFO FAR *LPCFR_LAST_PRINTED_INFO;

/* ����� ��᫥����� ���⯥�⠭���� ���㬥�� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrPrintLastDoc(WORD doc_type, LPCBYTE tmpl, SIZE_T tmpl_len,
		LPBYTE status, LPFR_LAST_PRINTED_INFO lpi, LPBYTE err_info, LPDWORD err_info_len);


/* ����� �ନ஢���� ���㬥�� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrBeginDoc(WORD doc_type, LPBYTE status, LPBYTE err_info, LPDWORD err_info_len);


/* ��।��� ����� ���㬥�� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrSendDocData(LPCBYTE data, SIZE_T len, LPBYTE status,
	LPBYTE err_info, LPDWORD err_info_len);


/* ���ଠ�� � ��ନ஢����� ���㬥�� */
typedef struct _FR_DOC_INFO {
	DWORD nr;
	DWORD sign;
} FR_DOC_INFO, FAR *LPFR_DOC_INFO;

typedef const FR_DOC_INFO FAR *LPCFR_DOC_INFO;

/* �������� �ନ஢���� ���㬥�� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrEndDoc(WORD doc_type, LPCBYTE tmpl, SIZE_T tmpl_len,
	LPBYTE status, LPFR_DOC_INFO doc_info, LPBYTE err_info, LPDWORD err_info_len);


/* ���/�६�, �ᯮ��㥬� � �� */
typedef struct _FR_FS_DATE {
	DWORD year;		/* ��� (���ਬ��, 2018) */
	DWORD month;		/* ����� (1 -- 12) */
	DWORD day;		/* ���� ����� (1 -- 31) */
} FR_FS_DATE, FAR *LPFR_FS_DATE;

typedef const FR_FS_DATE FAR *LPCFR_FS_DATE;

typedef struct _FR_FS_TIME {
	DWORD hour;		/* �� (0 -- 23) */
	DWORD minute;		/* ����� (0 -- 59) */
} FR_FS_TIME, FAR *LPFR_FS_TIME;

typedef const FR_FS_TIME FAR *LPCFR_FS_TIME;

typedef struct _FR_FS_DATE_TIME {
	FR_FS_DATE date;
	FR_FS_TIME time;
} FR_FS_DATE_TIME, FAR *LPFR_FS_DATE_TIME;

typedef const FR_FS_DATE_TIME FAR *LPCFR_FS_DATE_TIME;

/* ����� �� */
typedef struct _FR_FS_STATUS {
	DWORD life_state;		/* ���ﭨ� 䠧� ����� */
	DWORD current_doc;		/* ⥪�騩 ���㬥�� */
	DWORD doc_data;			/* ����� ���㬥�� */
	DWORD shift_state;		/* ���ﭨ� ᬥ�� */
	DWORD alert_flags;		/* 䫠�� �।�०����� */
	FR_FS_DATE_TIME date_time;	/* ��� � �६� ��᫥����� ���㬥�� */
#define FS_SERIAL_NUMBER_LEN	16
	TCHAR serial_number[FS_SERIAL_NUMBER_LEN + 1];	/* ����� �� */
	DWORD last_doc_number;		/* ����� ��᫥����� �� */
} FR_FS_STATUS, FAR *LPFR_FS_STATUS;

typedef const FR_FS_STATUS FAR *LPCFR_FS_STATUS;

/* ������� ����� �� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetFsStatus(LPBYTE status, LPFR_FS_STATUS fs_status);


/* ������� �����᪮� ����� �� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetFsNumber(LPBYTE status, LPTSTR fs_nr);


/* �ப ����⢨� �� */
typedef struct _FR_FS_LIFETIME {
	FR_FS_DATE_TIME dt;
	DWORD reg_complete;
	DWORD reg_remain;
} FR_FS_LIFETIME, FAR *LPFR_FS_LIFETIME;

typedef const FR_FS_LIFETIME FAR *LPCFR_FS_LIFETIME;

/* ������� �ப ����⢨� �� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetFsLifetime(LPBYTE status, LPFR_FS_LIFETIME fs_lifetime);


/* ����� �� */
typedef struct _FR_FS_VERSION {
#define FS_VERSION_LEN		16
	TCHAR version[FS_VERSION_LEN + 1];
	DWORD fw_type;
} FR_FS_VERSION, FAR *LPFR_FS_VERSION;

typedef const FR_FS_VERSION FAR *LPCFR_FS_VERSION;

/* ������� ����� �� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetFsVersion(LPBYTE status, LPFR_FS_VERSION fs_version);


/* ������� ���ଠ�� � ��᫥���� �訡��� �� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetFsError(LPBYTE status, LPBYTE data, LPDWORD data_len);


/* ��ࠬ���� ⥪�饩 ᬥ�� */
typedef struct _FR_SHIFT_PARAMS {
	BOOL opened;
	DWORD shift_nr;
	DWORD cheque_nr;
} FR_SHIFT_PARAMS, FAR *LPFR_SHIFT_PARAMS;

typedef const FR_SHIFT_PARAMS FAR *LPCFR_SHIFT_PARAMS;

/* ������� ��ࠬ���� ⥪�饩 ᬥ�� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetShiftParams(LPBYTE status, LPFR_SHIFT_PARAMS shift_params);


/* ����� ���ଠ樮����� ������ � ��� */
typedef struct _FR_FDO_TRANSMISSION_STATUS {
	DWORD transmission_state;
	BOOL msg_read_started;
	DWORD snd_queue_len;
	DWORD first_doc_nr;
	FR_FS_DATE_TIME first_doc_dt;
} FR_FDO_TRANSMISSION_STATUS, FAR *LPFR_FDO_TRANSMISSION_STATUS;

typedef const FR_FDO_TRANSMISSION_STATUS FAR *LPCFR_FDO_TRANSMISSION_STATUS;

/* ������� ����� ���ଠ樮����� ������ � ��� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetFdoTransmissionStatus(LPBYTE status, LPFR_FDO_TRANSMISSION_STATUS ts);


/* ���� �᪠���� ���㬥�⮢ */
enum class FrDocType : BYTE {
	None		= 0,	// ���
	Registration	= 1,	// ���� � ॣ����樨
	ReRegistration	= 11,	// ���� � ���ॣ����樨
	OpenShift	= 2,	// ����⨥ ᬥ��
	CalcReport	= 21,	// ���� � ���ﭨ� ���⮢
	Cheque		= 3,	// ���
	ChequeCorr	= 31,	// ��� ����樨
	BSO		= 4,	// ���
	BSOCorr		= 41,	// ��� ���४樨
	CloseShift	= 5,	// �����⨥ ᬥ��
	CloseFS		= 6,	// �����⨥ ��
	OpCommit	= 7,	// ���⢥ত���� ������
};

/* ���ଠ�� � ࠧ����� ⨯�� ���㬥�⮢ */
#pragma pack(push, 1)
/* ����� � ॣ����樨 (FrDocType::Registration) */
typedef struct _FR_REGISTER_REPORT {
	BYTE date_time[5];
	DWORD doc_nr;
	DWORD fiscal_sign;
	CHAR inn[12];
	CHAR reg_number[20];
	BYTE tax_system;
	BYTE mode;
} FR_REGISTER_REPORT, FAR *LPFR_REGISTER_REPORT;

typedef const FR_REGISTER_REPORT FAR *LPCFR_REGISTER_REPORT;

/* ����� � ���ॣ����樨 (FrDocType::ReRegistration) */
typedef struct _FR_REREGISTER_REPORT {
	BYTE date_time[5];
	DWORD doc_nr;
	DWORD fiscal_sign;
	CHAR inn[12];
	CHAR reg_number[20];
	BYTE tax_system;
	BYTE mode;
	BYTE rereg_code;
} FR_REREGISTER_REPORT, FAR *LPFR_REREGISTER_REPORT;

typedef const FR_REREGISTER_REPORT FAR *LPCFR_REREGISTER_REPORT;

/* ���/祪 ���४樨 (FrDocType::Cheque/FrDocType::ChequeCorr/FrDocType::BSO/FrDocType::BSOCorr) */
typedef struct _FR_CHEQUE_REPORT {
	BYTE date_time[5];
	DWORD doc_nr;
	DWORD fiscal_sign;
	BYTE type;
	BYTE sum[5];
} FR_CHEQUE_REPORT, FAR *LPFR_CHEQUE_REPORT;

typedef const FR_CHEQUE_REPORT FAR *LPCFR_CHEQUE_REPORT;

/* ����⨥/�����⨥ ᬥ�� (FrDocType::OpenShift/FrDocType::CloseShift) */
typedef struct _FR_SHIFT_REPORT {
	BYTE date_time[5];
	DWORD doc_nr;
	DWORD fiscal_sign;
	WORD shift_nr;
} FR_SHIFT_REPORT, FAR *LPFR_SHIFT_REPORT;

typedef const FR_SHIFT_REPORT FAR *LPCFR_SHIFT_REPORT;

/* �����⨥ �᪠�쭮�� ०��� (FrDocType::CloseFS) */
typedef struct _FR_CLOSE_FS {
	BYTE date_time[5];
	DWORD doc_nr;
	DWORD fiscal_sign;
	CHAR inn[12];
	CHAR reg_number[20];
} FR_CLOSE_FS, FAR *LPFR_CLOSE_FS;

typedef const FR_CLOSE_FS FAR *LPCFR_CLOSE_FS;

/* ����� � ���ﭨ� ����⮢ (FrDocType::CalcReport) */
typedef struct _FR_CALC_REPORT {
	BYTE dt[5];
	DWORD doc_nr;
	DWORD fiscal_sign;
	DWORD nr_uncommited;
	BYTE first_uncommited_dt[3];
} FR_CALC_REPORT, FAR *LPFR_CALC_REPORT;

typedef const FR_CALC_REPORT FAR *LPCFR_CALC_REPORT;

/* ���⠭�� ��� */
typedef struct _FR_FDO_ACK {
	BYTE dt[5];
	BYTE fiscal_sign[18];
	DWORD doc_nr;
} FR_FDO_ACK, FAR *LPFR_FDO_ACK;

typedef const FR_FDO_ACK FAR *LPCFR_FDO_ACK;
#pragma pack(pop)

/* ������� ���᪠ �᪠�쭮�� ���㬥�� �� ������ */
typedef struct _FR_FIND_DOC_INFO {
	WORD doc_type;
	BOOL fdo_ack;
} FR_FIND_DOC_INFO, FAR *LPFR_FIND_DOC_INFO;

typedef const FR_FIND_DOC_INFO FAR *LPCFR_FIND_DOC_INFO;

/* ���� �᪠��� ���㬥�� �� ������ */
TERMCORE_API TERMCORE_RETURN APIENTRY FrFindDocByNumber(DWORD doc_nr, BOOL need_print, LPBYTE status,
	LPFR_FIND_DOC_INFO fdi, LPBYTE data, LPDWORD data_len);

/* ���� ���⠭�� ��� �� ������ ���㬥�� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrFindFdoAck(DWORD ack_nr, BOOL need_print, LPBYTE status, LPFR_FDO_ACK fdo_ack);

/* �������� ������⢮ �᪠���� ���㬥�⮢, �� ����� ��� ���⠭樨 ��� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetUnconfirmedDocsNr(LPBYTE status, LPDWORD nr_unconfirmed);

/* ������� ����� ��᫥���� ॣ����樨 */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetLastRegData(LPBYTE status, LPBYTE data, LPDWORD data_len);

/* ���� �� � ��室��� ���ﭨ� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrResetFs(LPBYTE status);

/* ������ ������ ��� ��� */
TERMCORE_API TERMCORE_RETURN APIENTRY FrParseKktData(LPCBYTE data, SIZE_T len);

/******************************************************************************
 * ����஫쭠� ���� ��� (��3)                                                *
 ******************************************************************************/

/* �஢�� ��⠫���樨 ��3 */

/* ������ �ᥩ ���ଠ樨 */
#define KKT_LOG_ALL		0
/* ������� ��� ������ � ���� �����襭�� */
#define KKT_LOG_INF		1
/* ������� ��� ������ � ���� �����襭�� � ��砥 �訡�� */
#define KKT_LOG_WARN		2
/* ���� �����襭�� � ��砥 �訡�� */
#define KKT_LOG_ERR		3
/* ������ �� ��3 �� ������� */
#define KKT_LOG_OFF		4

/* ��⮪� ������ ��� */

/* ������� �ࠢ����� */
#define KKT_STREAM_CTL		0
/* ������� ���� */
#define KKT_STREAM_PRN		1
/* ����� � ��� */
#define KKT_STREAM_FDO		2
