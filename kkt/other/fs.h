#ifndef __FS_H_
#define __FS_H_

#include <stdint.h>
#include <stdbool.h>
#include "fs_hw.h"

// ��� �訡��
#define FS_RET_SUCCESS	0x00
// �������⭠� �������, ������ �ଠ� ���뫪� ��� ��������� ��ࠬ����.
// ������� � ⠪�� ����� �� �������. ��� �ଠ�, �����, ��⠢ (⨯, �ଠ�) ��ࠬ��஢ �� ᮮ⢥����� ᯥ�䨪�樨
#define FS_RET_UNKNOWN_CMD_OR_ARGS	0x01
// ����୮� ���ﭨ� ��
// ������ ������� �ॡ�� ��㣮�� ���ﭨ� ��
#define FS_RET_INVAL_STATE	0x02
// �訡�� ��
// ������� ���७�� ᢥ����� �� �訡��
#define FS_RET_FS_ERROR	0x03
// �訡�� �� (�ਯ⮣���᪨� ᮯ�����)
// ������� ���७�� ᢥ����� �� �訡��
#define FS_RET_CC_ERROR	0x04
// �����祭 �ப �ᯫ��樨 ��
#define FS_RET_LIFETIME_OVER	0x05
// ��娢 �� ��९�����
#define FS_RET_ARCHIVE_OVERFLOW	0x06
// ������ ��� �/��� �६�
// ��� � �६� ����樨 �� ᮮ⢥������ ������ ࠡ��� ��
#define FS_RET_INVALID_DATETIME	0x07
// ��� ����襭��� ������
// ����襭�� ����� ���������� � ��娢� ��
#define FS_RET_NO_DATA	0x08
// �����४⭮� ���祭�� ��ࠬ��஢ �������
// ��ࠬ���� ������� ����� �ࠢ���� �ଠ�, �� �� ���祭�� �� ��୮
#define FS_RET_INVAL_ARG_VALUE	0x09
// �ॢ�襭�� ࠧ��஢ TLV ������
// ������ ��।������� TLV ������ �ॢ�ᨫ �����⨬�
#define FS_RET_TLV_OVERSIZE	0x10
// 0Ah	�����४⭠� �������.
// (��� �⢥� �ନ����� ⮫쪮 � ��砥, �᫨ �� ��⨢���஢�� � ०��� �����প� ��� - 1.1)	
// � ������ ०��� �㭪樮��஢���� �� ������� �� ࠧ�襭�
#define FS_RET_INVALID_CMD	0x0a
// 0Bh	��ࠧ�襭�� ४������.
// (��� �⢥� �ନ����� ⮫쪮 � ��砥, �᫨ �� ��⨢���஢�� � ०��� �����প� ��� - 1.1)	
// �� �室�饬 ᮮ�饭�� ��� � ����� ������� 07h ��� ��।��� � �� �����, ����� ������ �ନ஢��� ��.
// ����� �����४⭮�� ४�����, ��।������ ��� � ��, ��।����� �� � ��� � ������ �⢥�(Uint16, LE)
#define FS_RET_ILLEGAL_ATTR	0x0b
// 0Ch	�㡫�஢���� ������	��� ��।��� �����, ����� 㦥 �뫨 ��।��� � ��⠢� ������� ���㬥��.
// ����� �㡫��㥬��� ४����� ��।����� � ������ �⢥�(Uint16, LE)
#define FS_RET_DUP_ATTR 0x0c
// 0Dh	���������� �����, ����室��� ��� ���४⭮�� ��� � ��.
// ��� ���४⭮�� ��� � �࠭���� �᪠���� ������, �ॡ���� ��।�� ��������� ������ � ��.
// ��� ������ �����襭�� 祪�� ����� ᮤ�ঠ�� ���.���� �⢥�, 㪠�뢠�騩 �� �������騥 �㬬� �� ࠧ�� ����� ������
#define FS_RET_MISS_ATTR 0x0d
// 0Eh	������⢮ ����権 � ���㬥��, �ॢ�ᨫ� �����⨬� �।��.
// �� ��।��� � ��� ��� ��� �⢥�, �᫨ ���ᨬ��쭮� �᫮ ����権, �ॢ�蠥� �����⨬� �।���
#define FS_RET_POS_OVERFLOW	0x0e
// ��� �࠭ᯮ�⭮�� ᮥ�������
// �࠭ᯮ�⭮� ᮥ������� (��) ���������. ����室��� ��⠭����� �� � ��� � ��।��� � �� ������� �࠭ᯮ�⭮� ᮥ������� � ���
#define FS_RET_NO_TRANSPORT	0x11
// ���௠� ����� �� (�ਯ⮣���᪮�� ᮯ�����)
// �ॡ���� �����⨥ �᪠�쭮�� ०���
#define FS_RET_CC_OUT	0x12
// ���௠� ����� �࠭����
// ������ ��� �࠭���� ���㬥�⮢ ��� ��� ���௠�
#define FS_RET_ARCHIVE_OUT	0x14
// ���௠� ����� �������� ��।�� ᮮ�饭��
// �६� ��宦����� � ��।� ᠬ��� ��ண� ᮮ�饭�� �� �뤠�� ����� 30
// ���������� ����. ���쪮 ��� ���� � ०��� ��।�� ������.
#define FS_RET_MSG_SEND_TIMEOUT	0x15
// �த����⥫쭮��� ᬥ�� ����� 24 �ᮢ
#define FS_RET_SHIFT_TIMEOUT	0x16
// ����ୠ� ࠧ��� �� �६��� ����� 2 �����ﬨ
// ������ ����� 祬 �� 5 ����� �⫨砥��� �� ࠧ���� ��।�������� �� ����७���� ⠩���� ��.
#define FS_RET_WRONG_PERIOD	0x17
// 18h
// �����४�� ४�����, ��।���� ��� � ��	��������, ��।���� ��� � ��, �� ᮮ⢥����� ��⠭������� �ॡ������.
// ����� �����४⭮�� ४����� ��।����� � ������ �⢥�(Uint16, LE)
#define FS_RET_INVALID_ATTR 0x18
// 19h
// �����४�� ४����� � �ਧ����� �த��� �����樧���� ⮢��.
// ��᪠��� ���㬥��, ��।���� � ��, ᮤ�ন� �ਧ��� �த��� �����樧���� ⮢��, 
// ���� � ॣ����樨 ��� ⥪�騩 ���� �� ��������� ��ࠬ��஢ ॣ����樨, �࠭�騩�� � ��,
// �� ᮤ�ন� �ਧ��� �த��� �����樧���� ⮢��
#define FS_RET_INVALID_ATTR_EXISABLE 0x19
// ����饭�� �� ��� �� ����� ���� �ਭ��
// ����饭�� ��� �� ����� ���� �ਭ��, ���७�� ����� �⢥� 㪠�뢠�� ��稭�
#define FS_RET_MSG_NOT_ACCEPTED	0x20

// ����� ������ � �⢥� ����� ���������
#define FS_RET_INVALID_RESP_DATA_SIZE	(0x21)

// ���
#pragma pack(push, 1)
typedef struct {
	uint8_t year;
	uint8_t month;
	uint8_t day;
} fs_date_t;
#pragma pack(pop)

// �६�
#pragma pack(push, 1)
typedef struct {
	uint8_t hour;
	uint8_t minute;
} fs_time_t;
#pragma pack(pop)

// ���/�६�
#pragma pack(push, 1)
typedef struct  {
	fs_date_t date;
	fs_time_t time;
} fs_date_time_t;
#pragma pack(pop)


// ������� 60h  ���� ���ﭨ� ��
// ������ ������� �������� �������� 䠧� ����� ��� ���ﭨ� ��. ������ ������� ����㯭� ⮫쪮 ��� �⫠��筮� ���ᨨ �� ��.
// ��ࠬ����: 16h (22)  ������ ���⪠ ��娢� � ��ॢ�� �� � 䠧� ����� ��⮢����� � �᪠����樨

#define FS_RESET_PARAM_CLEAR_ALL		0x16
extern fcore int fs_reset(uint8_t params, int32_t timeout);

//////////////////////////////////////////////////////////////
// ����� ���

// ����ன�� (0)
#define FS_LIFESTATE_NONE			0
// ��⮢����� � �᪠����樨 (1)
#define	FS_LIFESTATE_FREADY			1
// ��᪠��� ०�� (3)
#define	FS_LIFESTATE_FMODE			3
// �����᪠��� ०��, ���� ��।�� �� � ��� (7)
#define	FS_LIFESTATE_POST_FMODE			7
// �⥭�� ������ �� ��娢� �� (15)
#define	FS_LIFESTATE_ARCHIVE_READ		15

//////////////////////////////////////////////////////////////
// ����騩 ���㬥��

// 00h  ��� ����⮣� ���㬥��
#define FS_CURDOC_NOT_OPEN				0x00
// 01h  ����� � ॣ����樨 ���
#define FS_CURDOC_REG_REPORT				0x01
// 02h  ����� �� ����⨨ ᬥ��
#define FS_CURDOC_SHIFT_OPEN_REPORT			0x02
// 04h  ���ᮢ� 祪
#define FS_CURDOC_CHEQUE				0x04
// 08h  ����� � �����⨨ ᬥ��
#define FS_CURDOC_SHIFT_CLOSE_REPORT			0x08
// 10h  ����� � �����⨨ �᪠�쭮�� ०���
#define FS_CURDOC_FMODE_CLOSE_REPORT			0x10
// 11h  ����� ��ண�� ���⭮��
#define FS_CURDOC_BSO					0x11
// 12h - ���� �� ��������� ��ࠬ��஢ ॣ����樨 ��� � �裡 � ������� ��
#define FS_CURDOC_REG_PARAMS_REPORT_ON_FS_CHANGE	0x12
// 13h  ���� �� ��������� ��ࠬ��஢ ॣ����樨 ���
#define FS_CURDOC_REG_PARAMS_REPORT			0x13
// 14h  ���ᮢ� 祪 ���४樨
#define FS_CURDOC_CORRECTION_CHEQUE			0x14
// 15h  ��� ���४樨
#define FS_CURDOC_CORRECTION_BSO			0x15
// 17h  ���� � ⥪�饬 ���ﭨ� ���⮢
#define FS_CURDOC_CURRENT_PAY_REPORT			0x17

//////////////////////////////////////////////////////////////
// ����� ���㬥��

// 0  ��� ������ ���㬥��
#define FS_NO_DOC_DATA				0
// 1  ����祭� ����� ���㬥��
#define FS_HAS_DOC_DATA				1

//////////////////////////////////////////////////////////////
// ����ﭨ� ᬥ��
// 0  ᬥ�� ������
#define FS_SHIFT_CLOSED				0
// 1  ᬥ�� �����
#define FS_SHIFT_OPENED				1

//////////////////////////////////////////////////////////////
// �।�०����� �� ���௠��� ����ᮢ ��

// ��筠� ������ ��
#define FS_ALERT_CC_REPLACE_URGENT		0x01
// ���௠��� ����� ��
#define FS_ALERT_CC_EXHAUST			0x02
// ��९������� ����� (90% ���������)
#define FS_ALERT_MEMORY_FULL			0x04
// �ॢ�襭� �६� �������� �⢥� �� ���
#define FS_ALERT_RESP_TIMEOUT			0x08
// �⪠� �� ����� �ଠ⭮-�����᪮�� ����஫� (�ਧ��� ��।����� � ���⢥ত���� �� ���)
#define FS_ALERT_FLC				0x10
//�ॡ���� ����ன�� ��� (�ਧ��� ��।����� � ���⢥ত���� �� ���)
#define FS_ALERT_SETUP_REQUIRED			0x20
// ��� ���㫨஢��(�ਧ��� ��।����� � ���⢥ত���� �� ���)
#define FS_ALERT_OFD_REVOKED			0x40
// ����᪠� �訡�� ��
#define FS_ALERT_CRITRICAL			0x80

#pragma pack(push, 1)
typedef struct __PACKED__
{
	// ����ﭨ� 䠧� �����
	uint8_t life_state;
	// ����騩 ���㬥��
	uint8_t current_doc;
	// ����� ���㬥��
	uint8_t doc_data;
	// ����ﭨ� ᬥ��
	uint8_t shift_state;
	// ����� �।�०�����
	uint8_t alert_flags;
	// ��� � �६� ��᫥����� ���㬥��
	fs_date_time_t date_time;
	// ����� ��
#define FS_SERIAL_NUMBER_LEN	16
	char serial_number[FS_SERIAL_NUMBER_LEN];
	// ����� ��᫥����� ��
	uint32_t last_doc_number;
} fs_status_t;
#pragma pack(pop)

// ������� 30h  ����� ����� ��
// ������� 30h ��� �ᯮ���� ��� ����� ⥪�饣� ���ﭨ� ��.
extern fcore int fs_get_status(fs_status_t *status, int32_t timeout);
// ������� 31h  ����� ����� ��
extern fcore int fs_get_serial_number(char serialNumber[FS_SERIAL_NUMBER_LEN], int32_t timeout);

// ����� ������� (32h) (������� �ப ����⢨� ��)
#pragma pack(push, 1)
typedef struct
{
	fs_date_t date;
	uint8_t remaining_number_of_registrations;
	uint8_t number_of_registrations;
} fs_lifetime_t;
#pragma pack(pop)

// ������� 32h  ����� �ப� ����⢨� ��
extern fcore int fs_get_lifetime(fs_lifetime_t *data, int32_t timeout);

#define FS_VERSION_LEN	16
// ������� 33h  ����� ���ᨨ ��
extern fcore int fs_get_version(char version[FS_VERSION_LEN], uint8_t *type, int32_t timeout);

//////////////////////////////////////////////////////////////
// ������� 35h  ����� ��᫥���� �訡�� ��

typedef struct {
#define FS_MAX_ERROR_DETAIL_LEN	256
	uint8_t errorDetail[FS_MAX_ERROR_DETAIL_LEN];
	uint8_t size;
} fs_error_detail_t;

// ������� 35h  ����� ��᫥���� �訡�� ��
// ������� �������� ������� ���������᪨� ����� � ࠡ�� ��.����砥�� ����� ����室��� ��࠭��� ��� ���쭥�襩 ��।�� ࠧࠡ��稪�� ��.
extern fcore int fs_get_error_detail(fs_error_detail_t *result, int32_t timeout);

// ������� 06h  �⬥���� ���㬥��
// ������� �⬥��� �� ࠭�� ����� �᪠��� ���㬥��.�� �����, �������� � ������� ������� ��।��� ����� ���㬥�� 㤠������.
extern fcore int fs_cancel_document(int32_t timeout);

//////////////////////////////////////////////////////////////
// TLV 

// ������� 07h  ��।��� ����� ���㬥��
// ������� �।�����祭� ��� ��।�� �� ��� � �� ������ ���⮣� �᪠�쭮�� ���㬥��.����� ��।����� ��� ᯨ᮪ TLV ��ꥪ⮢.�� ����஫���� �ࠢ��쭮��� TLV(ᮮ⢥��⢨� ���� ��� 䠪��᪮� �����), ���⮬� ����室��� ��।����� ⮫쪮 楫� TLV ��ꥪ��(� ���� ����� ࠧ������ ���� ��ꥪ� �� 2 �������).
// �㬬�ୠ� ����� ��� ������, ������塞�� � ������� ������ �������, ������ �� ⨯� �᪠�쭮�� ���㬥�� � �ਢ������ � ���ᠭ�� ������� ����� ... ��� ������� �� �᪠���� ���㬥�⮢.
// �� �� �믮���� �ଠ⭮-�����᪨� ����஫� TLV ������.
// �����⨬� ��뢠�� ������ �㭪�� ��᪮�쪮 ࠧ, ��� ��।�� ��� ����室���� ������.
extern fcore int fs_send_document_data(const uint8_t tlv_data[], uint16_t size, int32_t timeout);


//////////////////////////////////////////////////////////////
// ������� 02h  ����� ���� � ॣ����樨 ��� (�᪠������ ��)

//////////////////////////////////////////////////////////////
// ���� � ॣ����樨
#define FS_REG_TYPE_REGISTRATION					0
// ���� �� ��������� ��ࠬ��஢ ॣ����樨 � �裡 � ������� ��
#define FS_REG_TYPE_UPDATE_WITH_FS_CHANGE		1
// ���� �� ��������� ��ࠬ��஢ ॣ����樨 ��� ������ ��
#define FS_REG_TYPE_UPDATE_WITHOUT_FS_CHANGE	2

// ������� 02h  ����� ���� � ॣ����樨 ��� (�᪠������ ��)
// ������� ��稭��� �ନ஢���� ������ �� ᫥����� ���⮢ :
//   ���� � ॣ����樨 ���
//   ���� �� ��������� ��ࠬ��஢ ॣ����樨 ���, � �裡 � ������� ��
//   ���� �� ��������� ��ࠬ��஢ ॣ����樨 ��� ��� ������ ��
// ��᫥ �믮������ �⮩ ������� �� ������� ����祭�� �������⥫��� ������ � ������� ������� ��।��� ����� ���㬥��.
// ���ᨬ���� ࠧ��� ��।������� ������ �� ����� �ॢ���� 2 ��������.
extern fcore int fs_begin_registration(uint8_t type, int32_t timeout);


// ����� ���㬥��
#pragma pack(push, 1)
typedef struct {
	// ����� ��
	uint32_t doc_no;
	// ��᪠��� �ਧ���
	uint32_t fiscal_sign;
} fs_doc_info_t;
#pragma pack(pop)

//////////////////////////////////////////////////////////////
// ������� 03h  ��ନ஢��� ���� � ॣ����樨 (���ॣ����樨) ���

//////////////////////////////////////////////////////////////
// ����஢�� ��⮢��� ���� ��� ���������������

// ����
#define FS_REG_TAXCODE_COMMON						0x1U
// ���饭��� ��室
#define FS_REG_TAXCODE_SIMPLE_INCOME				0x2U
// ���饭��� ��室 ����� ���室
#define FS_REG_TAXCODE_SIMPLE_INCOME_MINUS_EXPENSES	0x4U
// ����� ����� �� �������� ��室
#define FS_REG_TAXCODE_ENVD							0x8U
// ����� ᥫ�᪮宧��⢥��� �����
#define FS_REG_TAXCODE_AGRICULTURAL_TAX				0x10U
// ��⥭⭠� ��⥬� ���������������	
#define FS_REG_TAXCODE_PATENT						0x20U


//////////////////////////////////////////////////////////////
// ����஢�� ��⮢��� ���� ����� ࠡ���

// ���஢����
#define FS_REG_MODE_CRYPTO				0x1U
// ��⮭���� ०��
#define FS_REG_MODE_OFFLINE				0x2U
// ��⮬���᪨� ०��
#define FS_REG_MODE_AUTO				0x4U
// �ਬ������ � ��� ���
#define FS_REG_MODE_SERIVICE_SPHERE		0x8U
// ����� ���(1) ���� ����� 祪��(0)
#define FS_REG_MODE_BSO					0x10U
// �ਬ������ � ���୥�-�࣮���
#define FS_REG_MODE_INTERNET			0x20U

//////////////////////////////////////////////////////////////
// ���祭�� ���� ��� ��稭� ���ॣ����樨

// ������ �� (��� ��������� ��ࠬ��஢ ॣ����樨 ��� � �裡 � ������� ��, �� ���祭�� �㤥� ������ �� ��⮬���᪨)
#define FS_REG_REREG_FS_CHANGE	1
// ����� ���
#define FS_REG_REREG_OFD_CHANGE	2
// ����� ४����⮢ ���짮��⥫�
#define FS_REG_REREG_DETAILS_CHANGE	3
// ����� ����஥� ���
#define FS_REG_REREG_KKT_SETTINGS_CHANGE	4

#pragma pack(push, 1)
typedef struct {
	// ��� / �६� ॣ����樨
	fs_date_time_t date_time;
	// ���
	char inn[12];
	// ॣ����樮��� �����
	char reg_number[20];
	// ��� ���������������
	uint8_t tax_code;
	// ०�� ࠡ���
	uint8_t mode;
	// ��� ��稭� ���ॣ����樨 (�᫨ ��� ������ ��, � ������ ���� ������ ���� = 0
	uint8_t reregister_code;
} fs_registration_t;
#pragma pack(pop)

// ������� 03h  ��ନ஢��� ���� � ॣ����樨 (���ॣ����樨) ���
// ������ ������� �����蠥� �ନ஢���� ���� � ॣ����樨 ��� � ��ॢ���� �� � �᪠��� ०��.
// �� �� �맮�� ������ ���� �믮����� ������� ����� ���� � ॣ����樨 (���ॣ����樨) ��� � ��।��� ����� ���㬥��.
extern fcore int fs_end_registration(uint8_t type, fs_registration_t *data, fs_doc_info_t *result, int32_t timeout);

// ������� 04h  ����� �����⨥ �᪠�쭮�� ०��� ��
extern fcore int fs_begin_close_fiscal_mode(int32_t timeout);


//////////////////////////////////////////////////////////////
// ������� 05h  ������� �᪠��� ०�� ��

// ����� ��� ������� 05h  ������� �᪠��� ०�� ��
#pragma pack(push, 1)
typedef struct {
	//��� � �६�
	fs_date_time_t date_time;
	//�������樮��� ����� ���
	char reg_number[20];
} fs_close_fiscal_mode_t;
#pragma pack(pop)

// ������� 05h  ������� �᪠��� ०�� ��
// ������ ������� ����뢠�� ��᪠��� ०�� � ��ॢ���� �� � �����᪠��� ०��
extern fcore int fs_end_close_fiscal_mode(fs_close_fiscal_mode_t* data, fs_doc_info_t *result, int32_t timeout);


//////////////////////////////////////////////////////////////
// ������� 10h  ����� ��ࠬ��஢ ⥪�饩 ᬥ��

// ����� ��� ������� 10h  ����� ��ࠬ��஢ ⥪�饩 ᬥ��
#pragma pack(push, 1)
typedef struct {
	// ����ﭨ� ᬥ��
	// 0  ᬥ�� ������ 1  ᬥ�� �����
	bool is_opened;
	// ����� ᬥ��
	// �᫨ ᬥ�� ������, �  ����� ��᫥���� �����⮩ ᬥ��, �᫨ �����, � ����� ⥪�饩 ᬥ��.
	uint16_t shift_no;
	// ����� 祪�
	// �᫨ ᬥ�� ������, � �᫮ ���㬥�⮢ � �।��饩 �����⮩ ᬥ��(0, �᫨ �� ��ࢠ� ᬥ��).
	// �᫨ ᬥ�� �����, �� ��� �� ������ 祪�, � 0. 
	// � ��⠫��� �����  ����� ��᫥����� ��ନ஢������ 祪�
	uint16_t cheque_no;
} fs_current_shift_t;
#pragma pack(pop)

// ������� 10h  ����� ��ࠬ��஢ ⥪�饩 ᬥ��
// ������ ������� �������� 㧭��� ���ﭨ� ⥪�饩 ᬥ�� ���.
// �ᥣ�� �뤠���� ����� ⥪�饩 ᬥ�� (���� �᫨ ��� 㦥 ������), ���� �� �㤥� ����� ����� ᬥ��.
extern fcore int fs_get_current_shift_data(fs_current_shift_t* data, int32_t timeout);


// ������� 11h  ����� ����⨥ ᬥ��
// ������� ��稭��� ��楤��� ������ ᬥ��.
// ����� �᪠�쭮�� ���㬥�� ������ ���� ��।��� � ������� ������� ��।�� ������ ���㬥��.
// ���ᨬ���� ���� ������ 1 ��������.�᫮��� �믮������ : �� ������ ���� � �᪠�쭮� ०���.
// �६� ������ ᬥ�� ����� �� 1 �� ���⠢��� �� �६��� ������� �।��饩 ᬥ��(��� �६��� �᪠����樨 ��).
extern fcore int fs_begin_open_shift(fs_date_time_t* date_time, int32_t timeout);


//////////////////////////////////////////////////////////////
// ������� 12h  ������ ᬥ��

// १���� ������ ᬥ��
#pragma pack(push, 1)
typedef struct {
	// ����� ����� ����⮩ ᬥ��
	uint16_t shift_no;
	// ����� ���㬥��
	fs_doc_info_t doc_info;
	// 䫠�� �।�०����� (⮫쪮 1.1)
	uint8_t alert_flags;
} fs_open_shift_result_t;
#pragma pack(pop)

// ������� 12h  ������ ᬥ��
// �������, ��������� ��楤��� ������ ᬥ��.
// ���쪮 ��᫥ �믮������ ������ ������� �⠭������ �������묨 ��楤��� �ନ஢���� 祪�� � ������� ᬥ��.
// �᫮��� �믮������ : ������ ���� �믮����� ������� ����� ����⨥ ᬥ��; ������ ���� ��।��� ����� ���㬥��.
extern fcore int fs_end_open_shift(fs_open_shift_result_t* result, int32_t timeout);

// ������� 13h  ����� �����⨥ ᬥ��
// ������� ��稭��� ��楤��� ������� ᬥ��.
// �᫮��� �믮������ : ᬥ�� ������ ���� �����; 祪 ������ ���� ������; �� ������ ���� � �᪠�쭮� ०���.
// ����� �᪠�쭮�� ���㬥�� ������ ���� ��।��� � ������� ������� ��।�� ������ ���㬥��.
// ���ᨬ���� ���� ������ 1 ��������.
extern fcore int fs_begin_close_shift(fs_date_time_t* date_time, int32_t timeout);

//////////////////////////////////////////////////////////////
// ������� 14h  ������� ᬥ��
typedef fs_open_shift_result_t fs_close_shift_result_t;

// ������� 14h  ������� ᬥ��
// ������� �����蠥� ��楤��� ������� ᬥ��.
// �᫮��� �믮������ : ������ ���� �믮����� ������� ����� �����⨥ ᬥ�� � ��।��� ����� ���㬥��.
extern fcore int fs_end_close_shift(fs_close_shift_result_t* result, int32_t timeout);

// ������� 15h  ����� �ନ஢���� 祪�(���)
// ������� ��稭��� ��楤��� �ନ஢���� �᪠�쭮�� ���㬥�� ���ᮢ� 祪(��� ������ ��ண�� ���⭮��).�᫮��� �믮������ :
// ����� ������ ���� ����� � �� �� ���� ��㣮� 祪.
// ��� � �६� �� ������ �ॢ��室��� ����� 祬 �� 24 �� ���� � �६� ������ ������ ᬥ��.
// ����� 祪� ����室��� ��।��� � ������� ������� ��।��� ����� ���㬥��, ���ᨬ���� ���� ������ 祪� �� ����� �ॢ���� 30 ��������.
extern fcore int fs_begin_cheque(fs_date_time_t* date_time, int32_t timeout);

//////////////////////////////////////////////////////////////
//������� 16h  ��ନ஢��� 祪

// ��� ���⥦�
typedef enum {
	FS_PAY_TYPE_RECEIPT = 1,
	FS_PAY_TYPE_RETURN_RECEIPT = 2,
	FS_PAY_TYPE_EXPENSE = 3,
	FS_PAY_TYPE_RETURN_EXPENSE = 4,
	FS_PAY_TYPE_MAX = FS_PAY_TYPE_RETURN_EXPENSE
} fs_pay_type_t;

// १���� ������ ᬥ��
#pragma pack(push, 1)
typedef struct {
	//��� � �६�. ��।����� �६� ���, ���⠥��� �� 祪�
	fs_date_time_t date_time;
	//��� ����樨 1  ��室, 2  ������ ��室�, 3  ���室, 4  ������ ��室�
	uint8_t pay_type;
	// �⮣ 祪�. ��।����� �⮣���� �㬬�	祪�(� ��������).
	uint64_t sum;
} fs_end_cheque_t;
#pragma pack(pop)

// १���� ������ ᬥ��
#pragma pack(push, 1)
typedef struct {
	// ����� 祪� ����� ᬥ��
	uint16_t cheque_no;
	// ����� ���㬥��
	fs_doc_info_t doc_info;
} fs_end_cheque_result_t;
#pragma pack(pop)


//������� 16h  ��ନ஢��� 祪
//������� �������� ��᫥ ⮣�, ��� �� ����� 祪� �뫨 ��।��� � ������� ������� 15h ��� ������� 17h.
extern fcore int fs_end_cheque(fs_end_cheque_t *data, fs_end_cheque_result_t *result, int32_t timeout);

//������� 17h  ����� �ନ஢���� 祪� ���४樨(���).
//������� ��稭��� ��楤��� �ନ஢���� �᪠�쭮�� ���㬥�� ���ᮢ� 祪 ���४樨.�᫮��� �믮������ :
//  ����� ������ ���� ����� � �� �� ���� ��㣮� 祪(��� 祪 ���४樨).
//  ��� � �६� �� ������ �ॢ��室��� ����� 祬 �� 24 �� ���� � �६� ������ ������ ᬥ��.
// ����� 祪� ����室��� ��।��� � ������� ������� ��।��� ����� ���㬥��, ���ᨬ���� ���� ������ 祪� �� ����� �ॢ���� 30 ��������.
extern fcore int fs_begin_correction_cheque(fs_date_time_t* date_time, int32_t timeout);

// ������� 18h  ����� �ନ஢���� ���� � ���ﭨ� ���⮢
// ������� ��稭��� ��楤��� �ନ஢���� �᪠�쭮�� ���㬥�� ���� � ���ﭨ� ���⮢.�᫮��� �믮������ :
//   �� ������ ���� � ���ﭨ� ��᪠��� ०�� ��� ����䨪ᠫ�� ०��
//   ����� ������ ���� ������
//   ����� �᪠�쭮�� ���㬥�� ������ ���� ��।��� � ������� ������� ��।�� ������ ���㬥��, ���ᨬ���� ��ꥬ ������ 2 ��������
//   ���ᨬ���� ���� ������ 1 ��������.�᫮��� �믮������ : �� ������ ���� � �᪠�쭮� ०���.
extern fcore int fs_begin_calculation_report(fs_date_time_t* date_time, int32_t timeout);

///////////////////////////////////////////////////////////
// ������� 19h  ��ନ஢��� ���� � ���ﭨ� ���⮢

#pragma pack(push, 1)
typedef struct {
	// ����� ���㬥��
	fs_doc_info_t doc_info;
	// ��� - �� �����⢥ত����� ���㬥�⮢
	uint32_t unconfirmed_doc_count;
	// ��� ��ࢮ�� �����⢥ত������ ���㬥�� (��, ��, ��)
	fs_date_t first_unconfirmed_doc_date;
} fs_end_calculation_report_result_t;
#pragma pack(pop)

// ������� 19h  ��ନ஢��� ���� � ���ﭨ� ���⮢
// �������, ��������� ��楤��� �ନ஢���� ���� � ���ﭨ� ���⮢.
// �᫮��� �믮������ : ������ ���� �믮����� ������� ����� �ନ஢���� ���� � ���ﭨ� ���⮢
extern fcore int fs_end_calculation_report(fs_end_calculation_report_result_t *result, int32_t timeout);

//////////////////////////////////////////////////////////////////////////////////
// ������� ���ଠ樮����� ������ � ��ࢥ஬ ���
//

//////////////////////////////////////////////////////////////////////////////////
// ������� 20h  ������� ����� ���ଠ樮����� ������

// ��� 0  �࠭ᯮ�⭮� ᮥ������� ��⠭������
#define FS_TRANS_STATE_CONNECTED			0x1
// ��� 1  ���� ᮮ�饭�� ��� ��।�� � ���
#define FS_TRANS_STATE_HAS_REQ				0x2
// ��� 2  �������� �⢥⭮�� ᮮ�饭�� (���⠭樨) �� ���
#define FS_TRANS_STATE_WAIT_ACK				0x4
// ��� 3  ���� ������� �� ���
#define FS_TRANS_STATE_HAS_CMD				0x8
// ��� 4  ���������� ����ன�� ᮥ������� � ���
#define FS_TRANS_STATE_SETTINGS_CHANGED		0x10
// ��� 5  �������� �⢥�
#define FS_TRANS_STATE_WAIT_RESP			0x20

#pragma pack(push, 1)
typedef struct {
	// ����� ���ଠ樮����� ������
	uint8_t transmission_state;
	// ����ﭨ� �⥭�� ᮮ�饭�� ��� ��� (0 - �⥭�� �� ��砫���, 1 - �⥭�� ��砫���)
	uint8_t is_read_msg_started;
	// ������⢮ ᮮ�饭�� ��� ��।�� � ���
	uint16_t send_queue_count;
	// ����� ���㬥�� ��� ��� ��ࢮ�� � ��।�
	uint32_t first_doc_no;
	// ���-�६� ���㬥�� ��� ��� ��ࢮ�� � ��।�
	fs_date_time_t first_doc_date_time;
} fs_transmission_status_t;
#pragma pack(pop)


// ������� 20h  ������� ����� ���ଠ樮����� ������
// ������� ����訢��� ⥪�騩 ����� ���ଠ樮����� ������ � ��ࢥ஬ ���.
// �������� 㧭���, ���� �� ᮮ�饭�� ��� ��।�� � ��ࢥ� ���, ���� �� ������� �� ��ࢥ� ���,
// ����� ����� �� ���筮�� ������ ᮮ�饭�ﬨ ����� ������ ��� � ������ ��.
extern fcore int fs_get_transmission_status(fs_transmission_status_t *result, int32_t timeout);

// ������� 21h  ��।��� ����� �࠭ᯮ�⭮�� ᮥ������� � ��ࢥ஬ ���
// ������ ������� 㢥������ ������ �� �� ��⠭������� ��� ࠧ�뢥 �࠭ᯮ�⭮�� ᮥ������� � ��ࢥ஬ ���.
// 0  �࠭ᯮ�⭮� ᮥ������� ࠧ�ࢠ��
// 1  �࠭ᯮ�⭮� ᮥ������� ��⠭������
extern fcore int fs_set_connection_state(uint8_t is_connected, int32_t timeout);

// ������� 22h  ����� �⥭�� ����饭�� ��� ��ࢥ� ���
// ������ ������� ��稭��� �⥭�� ����饭�� ��� ���.
// ��᫥ �� �믮������ ��������  ������� �⥭�� ����� ����饭��, ������� �⬥�� �⥭�� ����饭�� ��� ������� �����襭�� �⥭�� ����饭��.
extern fcore int fs_begin_read_msg(uint16_t *msg_size, int32_t timeout);

// ������� 23h  ������ ���� ᮮ�饭�� ��� ��ࢥ� ���
// ������ ������� �।�����祭� ��� �����筮� ���⪨ ����饭�� ��� ��।�� � ���.
// ����� �⠥���� ����� � ᬥ饭�� ��।���� ���.
// �᫨ 䠪��᪨� ࠧ��� ������ ����� ����襭��� �����, � �㤥� �����饭 䠪��᪨� ࠧ��� ������.
// ���ᨬ��쭠� �⠥��� ����� �� ����� ���� ����� ���ᨬ��쭮�� ࠧ��� ������ ����饭��(�.�����᪨� ����䥩� ��).
extern fcore int fs_read_msg(uint16_t offset, uint8_t *data, uint16_t* size, int32_t timeout);

// ������� 24h  �⬥���� �⥭�� ����饭�� ��� ��ࢥ� ���
// �믮������ �⮩ ������� �⬥��� ������ ������ �⥭�� ᮮ�饭�� ��� ���.
extern fcore int fs_cancel_read_msg(int32_t timeout);

// ������� 25h  �������� �⥭�� ����饭�� ��� ��ࢥ� ���
// ������ ������� 㢥������ ��, �� ᮮ�饭�� ��� ��� �뫮 ��������� ����祭�.
extern fcore int fs_end_read_msg(int32_t timeout);

///////////////////////////////////////////////////////
//������� 26h  ��।��� ���⠭�� �� ��ࢥ� ���

// ��稭� �⪠�� (�᫨ � �⢥� �� 26h ������ ��� 0x20
// 1  ������ �᪠��� �ਧ���
#define FS_OFD_ASK_BAD_FISCAL_SIGN		1
// 2  ������ �ଠ� ���⠭樨
#define FS_OFD_ASK_BAD_FORMAT			2
// 3  ������ ����� ��
#define FS_OFD_ASK_BAD_FISCAL_DOC_NO	3
// 4  ������ ����� ��
#define FS_OFD_ASK_BAD_FS_NO			4
// 5  ������ CRC
#define FS_OFD_ASK_BAD_CRC				5

// ������� 26h  ��ࠡ���� ���⠭�� �� ��ࢥ� ���
// ������ ������� �।�����祭� ��� ��।�� � �� ����饭��(���⠭樨) �� ���.
// ����� ᮮ�饭�� �� ��� ��࠭�祭� ���ᨬ��쭮� ������ ������ � ����� ������� �� (�.�����᪨� ����䥩� ��).
extern fcore int fs_process_ofd_ack(uint8_t *data, uint16_t size, uint8_t *resp_code, uint8_t *stlv, uint16_t *stlv_size, int32_t timeout);


///////////////////////////////////////////////////////
// ������� 40h  ���� �᪠��� ���㬥�� �� ������

#pragma pack(push, 1)
typedef struct {
	// ��� ���㬥��
	uint8_t doc_type;
	// ����祭� �� ���⠭�� �� ��� 1 - ��, 0 - ���
	uint8_t has_ofd_ask;
} fs_find_result_t;
#pragma pack(pop)

// ������� 40h  ���� �᪠��� ���㬥�� �� ������
// ������� �������� ���� � ��娢� �� �᪠��� ���㬥�� �� ��� ������.
// ���㬥�� �뤠���� � �⢥⭮� ᮮ�饭��.
extern fcore int fs_find_doc_by_number(volatile uint32_t doc_no, fs_find_result_t *result,
                                 uint8_t *doc_data, uint16_t *doc_data_size, int32_t timeout);

// ������� 41h  ����� ���⠭樨 � ����祭�� �᪠�쭮�� ���㬥�� �᪠���� ������ � ��� �� ������ ���㬥��
// ������� �������� ���� � ��娢� �� ���⠭��, ���⢥ত����� ����祭�� �� � ���.

#pragma pack(push, 1)
typedef struct {
	// ��� � �६�
	fs_date_time_t date_time;
	// �᪠��� �ਧ��� ���
#define OFD_FISCAL_SIGN_SIZE    18
	uint8_t ofd_fiscal_sign[OFD_FISCAL_SIGN_SIZE];
	// ����� ��
	uint32_t doc_number;
} fs_ofd_ask_result_t;
#pragma pack(pop)
extern fcore int fs_get_ofd_ack_by_number(uint32_t doc_no, fs_ofd_ask_result_t *result, int32_t timeout);

//������� 42h  ����� ������⢠ ��, �� ����� ��� ���⠭樨
//������� �������� ������� ������⢮ ���㬥�⮢ � ��娢� ��, �� ����� �� ����祭� ���⠭樨 �� ���.
extern fcore int fs_get_doc_count_without_ofd_ack(uint16_t *count, int32_t timeout);

// ������� 43h  ����� �⮣�� �᪠����樨 ��
#pragma pack(push, 1)
typedef struct {
	// ����� ॣ����樨
	fs_registration_t reg;
	// ����� ���㬥��
	fs_doc_info_t doc_info;
} fs_registration_result_t;
#pragma pack(pop)

// ������� 43h  ����� �⮣�� �᪠����樨 ��
// docNumber : ���浪��� ����� ���� � ॣ����樨 (���ॣ����樨 ���), �᫨ -1, � ���� � �᪠����樨 ��ਠ�� 1
// type: ⨯ ��, 0 - �� � ०��� �����প� 1.0, 1 - �� � ०��� �����প� 1.1
extern fcore int fs_get_registration_result(int16_t number, fs_registration_result_t *result, int32_t timeout);

// ������� 44h  ����� ��ࠬ��� �᪠����樨 ��
// �������� ������� ���祭�� TLV ������ �� ��ࠬ��஢, �������� �� �᪠����樨 � ������� ��।��� ����� �᪠����樨.
// ����� ����㯭� ��� �뤠� ⮫쪮 ��᫥ �ᯥ譮�� �஢������ �᪠����樨.
extern fcore int fs_get_registration_parameter(int16_t number, uint16_t tag, void *data, uint32_t *size, int32_t timeout);



// ������� 45h  ����� �᪠�쭮�� ���㬥�� � TLV �ଠ�
// ������ ������� �������� ������ ��� ᮤ�ন��� �᪠�쭮�� ���㬥�� � TLV �ଠ�, ������ ����� ����祭�� �� ��� � �����, ��ନ஢���� ��.
#pragma pack(push, 1)
typedef struct {
	// ��
	uint16_t tag;
	// �����
	uint16_t length;
	// �����
#ifndef WIN32
	uint8_t data[];
#endif
} fs_tlv_t;
#pragma pack(pop)

extern fcore int fs_get_document_tlv(int32_t doc_no, fs_tlv_t *stlv, int32_t timeout);

// ������� 46h  �⥭�� TLV �᪠�쭮�� ���㬥��
// ������ ������� �।�����祭� ��� ����祭�� ������ �᪠�쭮�� ���㬥�� �� ��娢� ��.�� �믮������ �⮩ ������� ������ ���� �믮����� ������� 45h.
// ������� 46h ����室��� �������� �� �� ���, ���� �� �� �⢥�� ����� �訡�� 08h  ��� ����襭��� ������.
// � ������ �⢥� �� �� ������� 46h ᮤ�ন��� ⮫쪮 ���� ���� TLV ��� STLV ���孥�� �஢��.
extern fcore int fs_read_document_tlv(void *data, uint32_t *size, int32_t timeout);

// ������� 47h  �⥭�� TLV ��ࠬ��஢ �᪠����樨
// ������ ������� �।�����祭� ��� ����祭�� ��� ������, ��।����� ���, � ������� ������� 07h ��। �믮������� ������� 03h.
// ��। �ᯮ�짮������ ������ ������� ����室��� �맢��� ������� 44h ����� ��ࠬ��� �᪠����樨 �� � ���祭��� FFFFh � ����⢥ ��ண� ��ࠬ���.
// � ��⨢��� ��砥 ������� ��୥� ��� �訡�� 08h  ��� ����襭��� ������.
// ������� 47h ����室��� �������� �� �� ���, ���� �� �� �⢥�� ����� �訡�� 08h  ��� ����襭��� ������.
// � ������ �⢥� �� �� ������� 47h ᮤ�ন��� ⮫쪮 ���� ���� TLV ��� STLV ���孥�� �஢��.
extern fcore int fs_read_registration_tlv(void *data, uint32_t *size, int32_t timeout);

#endif
