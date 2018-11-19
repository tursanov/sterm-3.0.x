#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdefs.h"
#include "kkt/fd/tlv.h"
#include "kkt/fd/fd.h"
#include "kkt/kkt.h"
#include <limits.h>

static char last_error[1024] = { 0 };

#define DIR_NAME "/home/sterm/patterns"

static uint8_t *load_pattern(uint8_t doc_type, const uint8_t *pattern_footer,
	size_t pattern_footer_size, size_t *pattern_size)
{
#define PATTERN_FORMAT DIR_NAME "/%s"
	char file_name[PATH_MAX];
	FILE *f;
	int ret;
	long file_size;
	uint8_t *pattern = NULL;

	switch (doc_type) {
		case REGISTRATION:
		case RE_REGISTRATION:
			sprintf(file_name, PATTERN_FORMAT, "registration_pattern.dat");
			break;
		case OPEN_SHIFT:
			sprintf(file_name, PATTERN_FORMAT, "open_shift_pattern.dat");
			break;
		case CALC_REPORT:
			sprintf(file_name, PATTERN_FORMAT, "calc_report_pattern.dat");
			break;
		case CHEQUE:
		case BSO:
			sprintf(file_name, PATTERN_FORMAT, "cheque_pattern.dat");
			break;
		case CHEQUE_CORR:
		case BSO_CORR:
			sprintf(file_name, PATTERN_FORMAT, "cheque_corr_pattern.dat");
			break;
		case CLOSE_SHIFT:
			sprintf(file_name, PATTERN_FORMAT, "close_shift_pattern.dat");
			break;
		case CLOSE_FS:
			sprintf(file_name, PATTERN_FORMAT, "close_fs_pattern.dat");
			break;
	}

	printf("file_name: %s\n", file_name);

	if ((f = fopen(file_name, "rb")) == NULL) {
		perror("fopen");
		return NULL;
	}

	if ((ret = fseek(f, 0, SEEK_END)) != 0) {
		perror("fseek end");
		goto LOut;
	}
	file_size = ftell(f);
	if (file_size < 0) {
		perror("ftell");
		goto LOut;
	}
	if ((ret = fseek(f, 0, SEEK_SET)) != 0) {
		perror("fseek set");
		goto LOut;
	}
	printf("file_size: %ld\n", file_size);
	if ((pattern = (uint8_t *)malloc(file_size + pattern_footer_size + 1)) == NULL) {
		perror("malloc");
		goto LOut;
	}

	if ((ret = fread(pattern, file_size, 1, f) != 1)) {
		perror("read");
		free(pattern);
		goto LOut;
	}
	memcpy(pattern + pattern_footer_size, pattern_footer, pattern_footer_size);
	*pattern_size = (size_t)file_size + pattern_footer_size;

LOut:
	fclose(f);
	return pattern;
}

static void set_fn_error(char *s, uint8_t *err_info, size_t err_info_len)
{
	s += sprintf(s, "\nкоманда ФН: %.2x, код ошибки: %.2x\n", err_info[0], err_info[1]);
    switch (err_info[1]) {
        case 0x00: // FS_RET_SUCCESS
            sprintf(s, "%s", "Нет ошибок");
			break;
        case 0x01: // FS_RET_UNKNOWN_CMD_OR_ARGS
            sprintf(s, "%s", "Неизвестная команда, неверный формат посылки или неизвестные параметры.");
			break;
        case 0x02: // FS_RET_INVAL_STATE
            sprintf(s, "%s", "Неверное состояние ФН");
			break;
        case 0x03: // FS_RET_FS_ERROR
            sprintf(s, "%s", "Ошибка ФН");
			break;
        case 0x04: // FS_RET_CC_ERROR
            sprintf(s, "%s", "Ошибка КС (криптографический сопроцессор)");
			break;
        case 0x05: // FS_RET_LIFETIME_OVER
            sprintf(s, "%s", "Закончен срок эксплуатации ФН");
			break;
        case 0x06: // FS_RET_ARCHIVE_OVERFLOW
            sprintf(s, "%s", "Архив ФН переполнен");
			break;
        case 0x07: // FS_RET_INVALID_DATETIME
            sprintf(s, "%s", "Неверные дата и/или время");
			break;
        case 0x08: // FS_RET_NO_DATA
            sprintf(s, "%s", "Запрошенные данные отсутствуют в Архиве ФН");
			break;
        case 0x09: // FS_RET_INVAL_ARG_VALUE
            sprintf(s, "%s", "Параметры команды имеют правильный формат, но их значение не верно");
			break;
        case 0x10: // FS_RET_TLV_OVERSIZE
            sprintf(s, "%s", "Размер передаваемых TLV данных превысил допустимый");
			break;
        case 0x0a: // FS_RET_INVALID_CMD
            sprintf(s, "%s", "Некорректная команда.");
			break;
        case 0x0b: // FS_RET_ILLEGAL_ATTR
            sprintf(s, "%s", "Неразрешенные реквизиты.");
			break;
        case 0x0c: // FS_RET_DUP_ATTR
            sprintf(s, "%s", "Дублирование данных	ККТ передает данные, которые уже были переданы в составе данного документа.");
			break;
        case 0x0d: // FS_RET_MISS_ATTR
            sprintf(s, "%s", "Отсутствуют данные, необходимые для корректного учета в ФН.");
			break;
        case 0x0e: // FS_RET_POS_OVERFLOW
            sprintf(s, "%s", "Количество позиций в документе, превысило допустимый предел.");
			break;
        case 0x11: // FS_RET_NO_TRANSPORT
            sprintf(s, "%s", "Нет транспортного соединения");
			break;
        case 0x12: // FS_RET_CC_OUT
            sprintf(s, "%s", "Исчерпан ресурс КС (криптографического сопроцессора)");
			break;
        case 0x14: // FS_RET_ARCHIVE_OUT
            sprintf(s, "%s", "Исчерпан ресурс хранения");
			break;
        case 0x15: // FS_RET_MSG_SEND_TIMEOUT
            sprintf(s, "%s", "Исчерпан ресурс Ожидания передачи сообщения");
			break;
        case 0x16: // FS_RET_SHIFT_TIMEOUT
            sprintf(s, "%s", "Продолжительность смены более 24 часов");
			break;
        case 0x17: // FS_RET_WRONG_PERIOD
            sprintf(s, "%s", "Неверная разница во времени между 2 операциями");
			break;
        case 0x18: // FS_RET_INVALID_ATTR
            sprintf(s, "%s", "Некорректный реквизит, переданный ККТ в ФН	Реквизит, переданный ККТ в ФН, не соответствует установленным требованиям.");
			break;
        case 0x19: // FS_RET_INVALID_ATTR_EXISABLE
            sprintf(s, "%s", "Некорректный реквизит с признаком продажи подакцизного товара.");
			break;
        case 0x20: // FS_RET_MSG_NOT_ACCEPTED
            sprintf(s, "%s", "Сообщение от ОФД не может быть принято");
			break;
        case 0x21: // FS_RET_INVALID_RESP_DATA_SIZE
            sprintf(s, "%s", "Длина данных в ответе меньше ожидаемой");
			break;
        case 0x22: // FS_RET_INVAL
            sprintf(s, "%s", "Неправильное значение данных в команде");
			break;
        case 0x23: // FS_RET_TRANSPORT_ERROR
            sprintf(s, "%s", "Ошибка при передаче данных в ФН");
			break;
        case 0x24: // FS_RET_SEND_TIMEOUT
            sprintf(s, "%s", "Таймаут передачи в ФН");
			break;
        case 0x25: // FS_RET_RECV_TIMEOUT
            sprintf(s, "%s", "Таймайт приема из ФН");
			break;
        case 0x26: // FS_RET_CRC_ERROR
            sprintf(s, "%s", "Ошибка CRC");
			break;
        case 0x27: // FS_RET_WRITE_ERROR
            sprintf(s, "%s", "Ошибка записи в ФН");
			break;
        case 0x29: // FS_RET_READ_ERROR
            sprintf(s, "%s", "Ошибка приема из ФН");
			break;
        default:
            sprintf(s, "%s (0x%.2x)", "Неизвестная ошибка", err_info[1]);
			break;
    }
}

static void set_tag_error(char *s, uint8_t *err_info, size_t err_info_len) {
    s += sprintf(s, "\nтэг %.4d\n", *(uint16_t *)err_info);
    switch (err_info[2]) {
        case 0x01: // ERR_TAG_UNKNOWN
            sprintf(s, "%s", "Неизвестный для данного документа TLV");
            break;
        case 0x02: // ERR_TAG_NOT_FOR_INPUT
            sprintf(s, "%s", "Данный TLV заполняется ККТ и не должен передаваться");
            break;
        case 0x03: // ERR_TAG_STLV_FIELDS_NOT_FILLED
            sprintf(s, "%s", "Не все поля в STLV заполнены");
            break;
        case 0x04: // ERR_TAG_EMPTY
            sprintf(s, "%s", "TLV не заполнен (нулевая длина данных)");
            break;
        case 0x05: // ERR_TAG_TOO_LONG
            sprintf(s, "%s", "Длина TLV превышает допустимую");
            break;
        case 0x06: // ERR_TAG_LENGTH
            sprintf(s, "%s", "Неправильная длина TLV (Для данного TLV длина фиксирована)");
            break;
        case 0x07: // ERR_TAG_CONTENT
            sprintf(s, "%s", "Неправильное содержимое TLV");
            break;
        case 0x08: // ERR_TAG_METADATA
            sprintf(s, "%s", "Неправильное содержимое в метаданных");
            break;
        case 0x09: // ERR_TAG_NOT_IN_RANGE
            sprintf(s, "%s", "Значение TLV выходит за разрешенный диапазон");
            break;
        case 0x0a: // ERR_TAG_SAVE
            sprintf(s, "%s", "Ошибка при записи TLV в буфер для отправки в ФН");
            break;
        case 0x0b: // ERR_TAG_EXTRA
            sprintf(s, "%s", "Недопустимый для данного документа TLV");
            break;
        case 0x0c: // ERR_TAG_MISS
            sprintf(s, "%s", "Необходимый для данного документа TLV не был передан");
            break;
        case 0x0d: // ERR_TAG_INVALID_VALUE
            sprintf(s, "%s", "Неправильное значение TLV");
            break;
        case 0x0e: // ERR_TAG_DUPLICATE
            sprintf(s, "%s", "Дублирование тэга, который не может быть повторяющимся");
            break;
        case 0x20: // ERR_TAG_BEGIN_FB
            sprintf(s, "%s", "Ошибка при обработке начала фискального блока в шаблоне для печати");
            break;
        case 0x21: // ERR_TAG_END_FB
            sprintf(s, "%s", "Ошибка при обработке окончания фискального блока в шаблоне для печати");
            break;
        case 0x22: // ERR_TAG_TLV_HDR
            sprintf(s, "%s", "Ошибка при обработке заголовка TLV в шаблоне для печати");
            break;
        case 0x23: // ERR_TAG_TLV_VALUE
            sprintf(s, "%s", "Ошибка при обработке значения TLV в шаблоне для печати");
            break;
    }
}

static void set_error(uint8_t status, uint8_t *err_info, size_t err_info_len) 
{
	char *s = last_error;
	switch (status) {
		case 0x00: // STATUS_OK
		case 0x30:
			sprintf(s, "%s", "нет ошибок");
			break;
		case 0x41: // STATUS_PAPER_END
			sprintf(s, "%s", "конец бумаги");
		case 0x42: // STATUS_COVER_OPEN
			break;
			sprintf(s, "%s", "крышка открыта");
		case 0x43: // STATUS_PAPER_LOCK
			sprintf(s, "%s", "бумага застряла на выходе");
			break;
		case 0x44: // STATUS_PAPER_WRACK
			sprintf(s, "%s", "бумага замялась");
			break;
		case 0x45: // STATUS_FS_ERR
			s += sprintf(s, "%s", "общая аппаратная ошибка ФН");
			if (err_info_len > 0)
                set_fn_error(s, err_info, err_info_len);
			break;
		case 0x46: //
			sprintf(s, "%s", "последний сформированный документ не отпечатан");
			break;
		case 0x48: // STATUS_CUT_ERR
			sprintf(s, "%s", "ошибка отрезки бумаги");
			break;
		case 0x49: // STATUS_INIT
			sprintf(s, "%s", "ФР находится в состоянии инициализации");
			break;
		case 0x4a: // STATUS_THERMHEAD_ERR
			sprintf(s, "%s", "неполадки термоголовки");
			break;
		case 0x4d: // STATUS_PREV_INCOMPLETE
			sprintf(s, "%s", "предыдущая команда была принята не полностью");
			break;
		case 0x4e: // STATUS_CRC_ERR
			sprintf(s, "%s", "предыдущая команда была принята с ошибкой контрольной суммы");
			break;
		case 0x4f: // STATUS_HW_ERR
			sprintf(s, "%s", "общая аппаратная ошибка ФР");
			break;
		case 0x50: // STATUS_NO_FFEED
			sprintf(s, "%s", "нет команды отрезки бланка");
			break;
		case 0x51: // STATUS_VPOS_OVER
			sprintf(s, "%s", "превышение объёма текста по вертикальным позициям");
			break;
		case 0x52: // STATUS_HPOS_OVER
			sprintf(s, "%s", "превышение объёма текста по горизонтальным позициям");
			break;
		case 0x53: // STATUS_LOG_ERR
			sprintf(s, "%s", "нарушение структуры информации при печати КЛ");
			break;
		case 0x55: // STATUS_GRID_ERROR
			sprintf(s, "%s", "нарушение параметров нанесения макетов");
			break;
		case 0x70: // STATUS_BCODE_PARAM
			sprintf(s, "%s", "нарушение параметров нанесения штрих-кода");
			break;
		case 0x71: // STATUS_NO_ICON
			sprintf(s, "%s", "пиктограмма не найдена");
			break;
		case 0x72: // STATUS_GRID_WIDTH
			sprintf(s, "%s", "ширина сетки больше ширины бланка в установках");
			break;
		case 0x73: // STATUS_GRID_HEIGHT
			sprintf(s, "%s", "высота сетки больше высоты бланка в установках");
			break;
		case 0x74: // STATUS_GRID_NM_FMT
			sprintf(s, "%s", "неправильный формат имени сетки");
			break;
		case 0x75: // STATUS_GRID_NM_LEN
			sprintf(s, "%s", "длина имени сетки больше допустимой");
			break;
		case 0x76: // STATUS_GRID_NR
			sprintf(s, "%s", "неверный формат номера сетки");
			break;
		case 0x77: // STATUS_INVALID_ARG
			sprintf(s, "%s", "неверный параметр");
			break;
		case 0x80: // STATUS_INVALID_TAG
			s += sprintf(s, "%s", "Ошибка в TLV ");
            if (err_info_len > 0)
                set_tag_error(s, err_info, err_info_len);
			break;
		case 0x81: // STATUS_GET_TIME
			sprintf(s, "%s", "Ошибка при получении даты/времени");
			break;
		case 0x82: // STATUS_SEND_DOC
			sprintf(s, "%s", "Ошибка при передаче данных в ФН");
			break;
		case 0x83: // STATUS_TLV_OVERSIZE
			sprintf(s, "%s", "Выход за границы общей длины TLV-данных");
			break;
		case 0x84: // STATUS_INVALID_STATE
			sprintf(s, "%s", "неправильное состояние");
			break;
		case 0x86: // STATUS_SHIFT_NOT_CLOSED
			sprintf(s, "%s", "Смена не закрыта");
			break;
		case 0x87: // STATUS_SHIFT_NOT_OPENED
			sprintf(s, "%s", "Смена не открыта");
			break;
		case 0x88: // STATUS_NOMEM
			sprintf(s, "%s", "Не хватает памяти для завершения операции");
			break;
		case 0x89: // STATUS_SAVE_REG_DATA
			sprintf(s, "%s", "Ошибка записи регистрационных данных");
			break;
		case 0x8a: // STATUS_READ_REG_DATA
			sprintf(s, "%s", "Ошибка чтения регистрационных данных");
			break;
		case 0x8b: // STATUS_INVALID_INPUT
			sprintf(s, "%s", "Неправильный символ в шаблоне");
			break;
		case 0x8c: // STATUS_INVALID_TAG_IN_PATTERN
			s += sprintf(s, "%s", "Неправильный тэг в шаблоне ");
            if (err_info_len > 0)
                set_tag_error(s, err_info, err_info_len);
			break;
		case 0x8d: // STATUS_SHIFT_ALREADY_CLOSED
			sprintf(s, "%s", "Смена уже закрыта");
			break;
		case 0x8e: // STATUS_SHIFT_ALREADY_OPENED
			sprintf(s, "%s", "Смена уже открыта");
			break;
		case 0x85: // STATUS_FS_REPLACED
			sprintf(s, "%s", "Нельзя формировать ФД на другом ФН "
				"(допускается формирование ФД только на том ФН, "
				"с которым была проведена процедура регистрации "
				"(перерегистрации в связи с заменой ФН)");
			break;
		case 0xf1: // STATUS_TIMEOUT
			sprintf(s, "%s", "Таймаут приема ответа от ФР");
			break;
		case 0xf2:
			sprintf(s, "%s", "ККТ не обнаружена. Подключите ККТ");
			break;
		default:
			sprintf(s, "%s (0x%.2x)", "Неизвестная ошибка", status);
			break;
	}
}

int fd_create_doc(uint8_t doc_type, const uint8_t *pattern_footer, size_t pattern_footer_size)
{
	uint8_t ret;
	uint8_t err_info[32];
	size_t err_info_len;
	uint8_t *pattern;
	size_t pattern_size;
	struct kkt_doc_info di;

	err_info_len = sizeof(err_info);
	if ((ret = kkt_begin_doc(doc_type, err_info, &err_info_len)) != 0) {
		set_error(ret, err_info, err_info_len);
		printf("kkt_begin_doc->ret = %.2x\n", ret);
		return -1;
	}

	size_t tlv_size = ffd_tlv_size();
	printf("tlv_size = %d\n", tlv_size);
	if (tlv_size > 0) {
		uint8_t *tlv_data = ffd_tlv_data();
		const ffd_tlv_t *tlv = (const ffd_tlv_t *)tlv_data;
		const ffd_tlv_t *end = (const ffd_tlv_t *)(tlv_data + tlv_size);
		size_t tlv_buf_size = 0;
		uint8_t *tlv_buf = tlv_data;
	#define MAX_SEND_SIZE	4096

		while (tlv <= end) {
			if (tlv == end || tlv_buf_size + tlv->length > MAX_SEND_SIZE) {
				err_info_len = sizeof(err_info);

//				printf("tlv_buf_size = %d\n", tlv_buf_size);

				if ((ret = kkt_send_doc_data(tlv_buf, tlv_buf_size, err_info, &err_info_len)) != 0) {
					set_error(ret, err_info, err_info_len);
//					printf("kkt_send_doc_data->ret = %.2x\n", ret);

					if (ret == 0x80 || ret == 0x8c) {
						if (err_info_len > 0) {
							uint16_t tag = *(uint16_t *)err_info;
							uint8_t ex_err = err_info[2];

							printf("error in tag %.4d -> %.2x\n", tag, ex_err);
						}
					}

					return -1;
				}

				if (tlv == end)
					break;

				tlv_buf_size = 0;
				tlv_buf = (uint8_t *)tlv;
			}
			
/*			printf("tlv->tag: %d\n", tlv->tag);
			printf("tlv->length: %d\n", tlv->length);
			printf("  asString: %.*s\n", tlv->length, FFD_TLV_DATA_AS_STRING(tlv));
			printf("  as8: %d\n", FFD_TLV_DATA_AS_UINT8(tlv));
			if (tlv->length > 1)
				printf("  as16: %d\n", FFD_TLV_DATA_AS_UINT16(tlv));
			if (tlv->length > 3)
				printf("  as32: %d\n", FFD_TLV_DATA_AS_UINT32(tlv));*/
			
			tlv_buf_size += FFD_TLV_SIZE(tlv);
			tlv = FFD_TLV_NEXT(tlv);
		}

	}
	
	printf("------------------------------------\n");

   	if ((pattern = load_pattern(doc_type, pattern_footer,
				   	pattern_footer_size, &pattern_size)) == NULL) {
		printf("load_pattern fail\n");
		return -1;
	}

	err_info_len = sizeof(err_info);
	if ((ret = kkt_end_doc(doc_type, pattern, pattern_size, &di, err_info, &err_info_len)) != 0) {
		set_error(ret, err_info, err_info_len);
		printf("kkt_end_doc->ret = %.2x, err_info_len = %d\n", ret, err_info_len);

		if (ret == 0x80 || ret == 0x8c) {
			if (err_info_len > 0) {
				uint16_t tag = *(uint16_t *)err_info;
				uint8_t ex_err = err_info[2];

				printf("error in tag %.4d -> %.2x\n", tag, ex_err);
			}
		}

		goto LOut;
	}

LOut:
	free(pattern);

	return ret != 0 ? -1 : 0;
}

int fd_get_last_error(const char **error)
{
	*error = last_error;
	return 0;
}

