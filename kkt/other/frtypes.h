/* Типы и константы для работы с ФР. (c) gsr, 2018 */

#pragma once

#include "termcore.h"

/* Получить инструкцию по обмену с ОФД */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetFdoCmd(BYTE prev_op, WORD prev_op_status, LPBYTE cmd,
	LPBYTE data, LPDWORD data_len);


/* Передать в ФР данные, полученные от ОФД */
TERMCORE_API TERMCORE_RETURN APIENTRY FrSendFdoData(LPCBYTE data, DWORD data_len, LPBYTE status);


/* Информация часов реального времени */
typedef struct _FR_RTC_DATA {
	DWORD year;		/* год (например, 2019) */
	DWORD month;		/* месяц (1 -- 12) */
	DWORD day;		/* день (1 -- 31) */
	DWORD hour;		/* час (0 -- 23) */
	DWORD minute;		/* минута (0 -- 59) */
	DWORD second;		/* секунда (0 -- 59) */
} FR_RTC_DATA, FAR *LPFR_RTC_DATA;

typedef const FR_RTC_DATA FAR *LPCFR_RTC_DATA;


/* Информация о последнем сформированном и последнем отпечатанном фискальных документах */
typedef struct _FR_LAST_DOC_INFO {
	DWORD last_nr;
	WORD last_type;
	DWORD last_printed_nr;
	WORD last_printed_type;
} FR_LAST_DOC_INFO, FAR *LPFR_LAST_DOC_INFO;

typedef const FR_LAST_DOC_INFO FAR *LPCFR_LAST_DOC_INFO;

/* Получить данные о последнем сформированном и последнем отпечатанном фискальных документах */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetLastDocInfo(LPBYTE status, LPFR_LAST_DOC_INFO ldi,
		LPBYTE err_info, LPDWORD err_info_len);


/* Информация о последнм отпечатанном документе */
typedef struct _FR_LAST_PRINTED_INFO {
	DWORD nr;
	DWORD sign;
	WORD type;
} FR_LAST_PRINTED_INFO, FAR *LPFR_LAST_PRINTED_INFO;

typedef const FR_LAST_PRINTED_INFO FAR *LPCFR_LAST_PRINTED_INFO;

/* Печать последнего неотпечатанного документа */
TERMCORE_API TERMCORE_RETURN APIENTRY FrPrintLastDoc(WORD doc_type, LPCBYTE tmpl, SIZE_T tmpl_len,
		LPBYTE status, LPFR_LAST_PRINTED_INFO lpi, LPBYTE err_info, LPDWORD err_info_len);


/* Начать формирование документа */
TERMCORE_API TERMCORE_RETURN APIENTRY FrBeginDoc(WORD doc_type, LPBYTE status, LPBYTE err_info, LPDWORD err_info_len);


/* Передать данные документа */
TERMCORE_API TERMCORE_RETURN APIENTRY FrSendDocData(LPCBYTE data, SIZE_T len, LPBYTE status,
	LPBYTE err_info, LPDWORD err_info_len);


/* Информация о сформированном документе */
typedef struct _FR_DOC_INFO {
	DWORD nr;
	DWORD sign;
} FR_DOC_INFO, FAR *LPFR_DOC_INFO;

typedef const FR_DOC_INFO FAR *LPCFR_DOC_INFO;

/* Завершить формирование документа */
TERMCORE_API TERMCORE_RETURN APIENTRY FrEndDoc(WORD doc_type, LPCBYTE tmpl, SIZE_T tmpl_len,
	LPBYTE status, LPFR_DOC_INFO doc_info, LPBYTE err_info, LPDWORD err_info_len);


/* Дата/время, используемые в ФН */
typedef struct _FR_FS_DATE {
	DWORD year;		/* год (например, 2018) */
	DWORD month;		/* месяц (1 -- 12) */
	DWORD day;		/* день месяца (1 -- 31) */
} FR_FS_DATE, FAR *LPFR_FS_DATE;

typedef const FR_FS_DATE FAR *LPCFR_FS_DATE;

typedef struct _FR_FS_TIME {
	DWORD hour;		/* час (0 -- 23) */
	DWORD minute;		/* минута (0 -- 59) */
} FR_FS_TIME, FAR *LPFR_FS_TIME;

typedef const FR_FS_TIME FAR *LPCFR_FS_TIME;

typedef struct _FR_FS_DATE_TIME {
	FR_FS_DATE date;
	FR_FS_TIME time;
} FR_FS_DATE_TIME, FAR *LPFR_FS_DATE_TIME;

typedef const FR_FS_DATE_TIME FAR *LPCFR_FS_DATE_TIME;

/* Статус ФН */
typedef struct _FR_FS_STATUS {
	DWORD life_state;		/* состояние фазы жизни */
	DWORD current_doc;		/* текущий документ */
	DWORD doc_data;			/* данные документа */
	DWORD shift_state;		/* состояние смены */
	DWORD alert_flags;		/* флаги предупреждения */
	FR_FS_DATE_TIME date_time;	/* дата и время последнего документа */
#define FS_SERIAL_NUMBER_LEN	16
	TCHAR serial_number[FS_SERIAL_NUMBER_LEN + 1];	/* номер ФН */
	DWORD last_doc_number;		/* номер последнего ФД */
} FR_FS_STATUS, FAR *LPFR_FS_STATUS;

typedef const FR_FS_STATUS FAR *LPCFR_FS_STATUS;

/* Получить статус ФН */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetFsStatus(LPBYTE status, LPFR_FS_STATUS fs_status);


/* Получить заводской номер ФН */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetFsNumber(LPBYTE status, LPTSTR fs_nr);


/* Срок действия ФН */
typedef struct _FR_FS_LIFETIME {
	FR_FS_DATE_TIME dt;
	DWORD reg_complete;
	DWORD reg_remain;
} FR_FS_LIFETIME, FAR *LPFR_FS_LIFETIME;

typedef const FR_FS_LIFETIME FAR *LPCFR_FS_LIFETIME;

/* Получить срок действия ФН */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetFsLifetime(LPBYTE status, LPFR_FS_LIFETIME fs_lifetime);


/* Версия ФН */
typedef struct _FR_FS_VERSION {
#define FS_VERSION_LEN		16
	TCHAR version[FS_VERSION_LEN + 1];
	DWORD fw_type;
} FR_FS_VERSION, FAR *LPFR_FS_VERSION;

typedef const FR_FS_VERSION FAR *LPCFR_FS_VERSION;

/* Получить версию ФН */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetFsVersion(LPBYTE status, LPFR_FS_VERSION fs_version);


/* Получить информацию о последних ошибках ФН */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetFsError(LPBYTE status, LPBYTE data, LPDWORD data_len);


/* Параметры текущей смены */
typedef struct _FR_SHIFT_PARAMS {
	BOOL opened;
	DWORD shift_nr;
	DWORD cheque_nr;
} FR_SHIFT_PARAMS, FAR *LPFR_SHIFT_PARAMS;

typedef const FR_SHIFT_PARAMS FAR *LPCFR_SHIFT_PARAMS;

/* Получить параметры текущей смены */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetShiftParams(LPBYTE status, LPFR_SHIFT_PARAMS shift_params);


/* Статус информационного обмена с ОФД */
typedef struct _FR_FDO_TRANSMISSION_STATUS {
	DWORD transmission_state;
	BOOL msg_read_started;
	DWORD snd_queue_len;
	DWORD first_doc_nr;
	FR_FS_DATE_TIME first_doc_dt;
} FR_FDO_TRANSMISSION_STATUS, FAR *LPFR_FDO_TRANSMISSION_STATUS;

typedef const FR_FDO_TRANSMISSION_STATUS FAR *LPCFR_FDO_TRANSMISSION_STATUS;

/* Получить статус информационного обмена с ОФД */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetFdoTransmissionStatus(LPBYTE status, LPFR_FDO_TRANSMISSION_STATUS ts);


/* Типы фискальных документов */
enum class FrDocType : BYTE {
	None		= 0,	// Нет
	Registration	= 1,	// Отчет о регистрации
	ReRegistration	= 11,	// Отчет о перерегистрации
	OpenShift	= 2,	// Открытие смены
	CalcReport	= 21,	// Отчет о состоянии расчетов
	Cheque		= 3,	// Чек
	ChequeCorr	= 31,	// Чек корреции
	BSO		= 4,	// БСО
	BSOCorr		= 41,	// БСО коррекции
	CloseShift	= 5,	// Закрытие смены
	CloseFS		= 6,	// Закрытие ФН
	OpCommit	= 7,	// Подтверждение оператора
};

/* Информация о различных типах документов */
#pragma pack(push, 1)
/* Отчёт о регистрации (FrDocType::Registration) */
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

/* Отчёт о перерегистрации (FrDocType::ReRegistration) */
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

/* Чек/чек коррекции (FrDocType::Cheque/FrDocType::ChequeCorr/FrDocType::BSO/FrDocType::BSOCorr) */
typedef struct _FR_CHEQUE_REPORT {
	BYTE date_time[5];
	DWORD doc_nr;
	DWORD fiscal_sign;
	BYTE type;
	BYTE sum[5];
} FR_CHEQUE_REPORT, FAR *LPFR_CHEQUE_REPORT;

typedef const FR_CHEQUE_REPORT FAR *LPCFR_CHEQUE_REPORT;

/* Открытие/закрытие смены (FrDocType::OpenShift/FrDocType::CloseShift) */
typedef struct _FR_SHIFT_REPORT {
	BYTE date_time[5];
	DWORD doc_nr;
	DWORD fiscal_sign;
	WORD shift_nr;
} FR_SHIFT_REPORT, FAR *LPFR_SHIFT_REPORT;

typedef const FR_SHIFT_REPORT FAR *LPCFR_SHIFT_REPORT;

/* Закрытие фискального режима (FrDocType::CloseFS) */
typedef struct _FR_CLOSE_FS {
	BYTE date_time[5];
	DWORD doc_nr;
	DWORD fiscal_sign;
	CHAR inn[12];
	CHAR reg_number[20];
} FR_CLOSE_FS, FAR *LPFR_CLOSE_FS;

typedef const FR_CLOSE_FS FAR *LPCFR_CLOSE_FS;

/* Отчёт о состоянии расчётов (FrDocType::CalcReport) */
typedef struct _FR_CALC_REPORT {
	BYTE dt[5];
	DWORD doc_nr;
	DWORD fiscal_sign;
	DWORD nr_uncommited;
	BYTE first_uncommited_dt[3];
} FR_CALC_REPORT, FAR *LPFR_CALC_REPORT;

typedef const FR_CALC_REPORT FAR *LPCFR_CALC_REPORT;

/* Квитанция ОФД */
typedef struct _FR_FDO_ACK {
	BYTE dt[5];
	BYTE fiscal_sign[18];
	DWORD doc_nr;
} FR_FDO_ACK, FAR *LPFR_FDO_ACK;

typedef const FR_FDO_ACK FAR *LPCFR_FDO_ACK;
#pragma pack(pop)

/* Результат поиска фискального документа по номеру */
typedef struct _FR_FIND_DOC_INFO {
	WORD doc_type;
	BOOL fdo_ack;
} FR_FIND_DOC_INFO, FAR *LPFR_FIND_DOC_INFO;

typedef const FR_FIND_DOC_INFO FAR *LPCFR_FIND_DOC_INFO;

/* Найти фискальный документ по номеру */
TERMCORE_API TERMCORE_RETURN APIENTRY FrFindDocByNumber(DWORD doc_nr, BOOL need_print, LPBYTE status,
	LPFR_FIND_DOC_INFO fdi, LPBYTE data, LPDWORD data_len);

/* Найти квитанцию ОФД по номеру документа */
TERMCORE_API TERMCORE_RETURN APIENTRY FrFindFdoAck(DWORD ack_nr, BOOL need_print, LPBYTE status, LPFR_FDO_ACK fdo_ack);

/* Подсчитать количество фискальных документов, на которые нет квитанции ОФД */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetUnconfirmedDocsNr(LPBYTE status, LPDWORD nr_unconfirmed);

/* Получить данные последней регистрации */
TERMCORE_API TERMCORE_RETURN APIENTRY FrGetLastRegData(LPBYTE status, LPBYTE data, LPDWORD data_len);

/* Сброс ФН в исходное состояние */
TERMCORE_API TERMCORE_RETURN APIENTRY FrResetFs(LPBYTE status);

/* Разбор данных для ККТ */
TERMCORE_API TERMCORE_RETURN APIENTRY FrParseKktData(LPCBYTE data, SIZE_T len);

/******************************************************************************
 * Контрольная лента ККТ (КЛ3)                                                *
 ******************************************************************************/

/* Уровни детализации КЛ3 */

/* Запись всей информации */
#define KKT_LOG_ALL		0
/* Команды без данных и коды завершения */
#define KKT_LOG_INF		1
/* Команды без данных и коды завершения в случае ошибок */
#define KKT_LOG_WARN		2
/* Коды завершения в случае ошибок */
#define KKT_LOG_ERR		3
/* Запись на КЛ3 не ведётся */
#define KKT_LOG_OFF		4

/* Потоки данных ККТ */

/* Команды управления */
#define KKT_STREAM_CTL		0
/* Команды печати */
#define KKT_STREAM_PRN		1
/* Обмен с ОФД */
#define KKT_STREAM_FDO		2
