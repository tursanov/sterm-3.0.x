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

static int set_error(uint8_t status, uint8_t *err_info, size_t err_info_len) 
{
	switch (status) {
		case 0x00: // STATUS_OK
		case 0x30:
			sprintf(last_error, "%s", "нет ошибок");
			break;
		case 0x41: // STATUS_PAPER_END
			sprintf(last_error, "%s", "конец бумаги");
		case 0x42: // STATUS_COVER_OPEN
			break;
			sprintf(last_error, "%s", "крышка открыта");
		case 0x43: // STATUS_PAPER_LOCK
			sprintf(last_error, "%s", "бумага застряла на выходе");
			break;
		case 0x44: // STATUS_PAPER_WRACK
			sprintf(last_error, "%s", "бумага замялась");
			break;
		case 0x45: // STATUS_FS_ERR
/*			string ex_err;
			if (err_info_len > 0) {
				ex_err = string.Format("(Код команды: {0:X2}h, Код ошибки: {1:X2}h): {2}", err_info[0], err_info[1], GetFNError(err_info[1]));
			} else
				ex_err = "";*/
			sprintf(last_error, "%s", "общая аппаратная ошибка ФН");
			break;
		case 0x46: //
			sprintf(last_error, "%s", "последний сформированный документ не отпечатан");
			break;
		case 0x48: // STATUS_CUT_ERR
			sprintf(last_error, "%s", "ошибка отрезки бумаги");
			break;
		case 0x49: // STATUS_INIT
			sprintf(last_error, "%s", "ФР находится в состоянии инициализации");
			break;
		case 0x4a: // STATUS_THERMHEAD_ERR
			sprintf(last_error, "%s", "неполадки термоголовки");
			break;
		case 0x4d: // STATUS_PREV_INCOMPLETE
			sprintf(last_error, "%s", "предыдущая команда была принята не полностью");
			break;
		case 0x4e: // STATUS_CRC_ERR
			sprintf(last_error, "%s", "предыдущая команда была принята с ошибкой контрольной суммы");
			break;
		case 0x4f: // STATUS_HW_ERR
			sprintf(last_error, "%s", "общая аппаратная ошибка ФР");
			break;
		case 0x50: // STATUS_NO_FFEED
			sprintf(last_error, "%s", "нет команды отрезки бланка");
			break;
		case 0x51: // STATUS_VPOS_OVER
			sprintf(last_error, "%s", "превышение объёма текста по вертикальным позициям");
			break;
		case 0x52: // STATUS_HPOS_OVER
			sprintf(last_error, "%s", "превышение объёма текста по горизонтальным позициям");
			break;
		case 0x53: // STATUS_LOG_ERR
			sprintf(last_error, "%s", "нарушение структуры информации при печати КЛ");
			break;
		case 0x55: // STATUS_GRID_ERROR
			sprintf(last_error, "%s", "нарушение параметров нанесения макетов");
			break;
		case 0x70: // STATUS_BCODE_PARAM
			sprintf(last_error, "%s", "нарушение параметров нанесения штрих-кода");
			break;
		case 0x71: // STATUS_NO_ICON
			sprintf(last_error, "%s", "пиктограмма не найдена");
			break;
		case 0x72: // STATUS_GRID_WIDTH
			sprintf(last_error, "%s", "ширина сетки больше ширины бланка в установках");
			break;
		case 0x73: // STATUS_GRID_HEIGHT
			sprintf(last_error, "%s", "высота сетки больше высоты бланка в установках");
			break;
		case 0x74: // STATUS_GRID_NM_FMT
			sprintf(last_error, "%s", "неправильный формат имени сетки");
			break;
		case 0x75: // STATUS_GRID_NM_LEN
			sprintf(last_error, "%s", "длина имени сетки больше допустимой");
			break;
		case 0x76: // STATUS_GRID_NR
			sprintf(last_error, "%s", "неверный формат номера сетки");
			break;
		case 0x77: // STATUS_INVALID_ARG
			sprintf(last_error, "%s", "неверный параметр");
			break;
		case 0x80: // STATUS_INVALID_TAG
/*			if (err_info_len > 0)
				ex_err = GetTagError(err_info, err_info_len);
			else
				ex_err = "";*/
			sprintf(last_error, "%s", "Ошибка в TLV ");
			break;
		case 0x81: // STATUS_GET_TIME
			sprintf(last_error, "%s", "Ошибка при получении даты/времени");
			break;
		case 0x82: // STATUS_SEND_DOC
			sprintf(last_error, "%s", "Ошибка при передаче данных в ФН");
			break;
		case 0x83: // STATUS_TLV_OVERSIZE
			sprintf(last_error, "%s", "Выход за границы общей длины TLV-данных");
			break;
		case 0x84: // STATUS_INVALID_STATE
			sprintf(last_error, "%s", "неправильное состояние");
			break;
		case 0x86: // STATUS_SHIFT_NOT_CLOSED
			sprintf(last_error, "%s", "Смена не закрыта");
			break;
		case 0x87: // STATUS_SHIFT_NOT_OPENED
			sprintf(last_error, "%s", "Смена не открыта");
			break;
		case 0x88: // STATUS_NOMEM
			sprintf(last_error, "%s", "Не хватает памяти для завершения операции");
			break;
		case 0x89: // STATUS_SAVE_REG_DATA
			sprintf(last_error, "%s", "Ошибка записи регистрационных данных");
			break;
		case 0x8a: // STATUS_READ_REG_DATA
			sprintf(last_error, "%s", "Ошибка чтения регистрационных данных");
			break;
		case 0x8b: // STATUS_INVALID_INPUT
			sprintf(last_error, "%s", "Неправильный символ в шаблоне");
			break;
		case 0x8c: // STATUS_INVALID_TAG_IN_PATTERN
/*			if (err_info_len > 0)
				ex_err = GetTagError(err_info, err_info_len);
			else
				ex_err = "";*/
			sprintf(last_error, "%s", "Неправильный тэг в шаблоне ");
			break;
		case 0x8d: // STATUS_SHIFT_ALREADY_CLOSED
			sprintf(last_error, "%s", "Смена уже закрыта");
			break;
		case 0x8e: // STATUS_SHIFT_ALREADY_OPENED
			sprintf(last_error, "%s", "Смена уже открыта");
			break;
		case 0x85: // STATUS_FS_REPLACED
			sprintf(last_error, "%s", "Нельзя формировать ФД на другом ФН "
				"(допускается формирование ФД только на том ФН, "
				"с которым была проведена процедура регистрации "
				"(перерегистрации в связи с заменой ФН)");
			break;
		case 0xf1: // STATUS_TIMEOUT
			sprintf(last_error, "%s", "Таймаут приема ответа от ФР");
			break;
		default:
			sprintf(last_error, "%s", "неизвестная ошибка");
			break;
	}

	return 0;
}

int fd_create_doc(uint8_t doc_type, const uint8_t *pattern_footer, size_t pattern_footer_size)
{
	uint8_t ret;
	uint8_t err_info[32];
	size_t err_info_len = sizeof(err_info);
	uint8_t *pattern;
	size_t pattern_size;
	struct kkt_doc_info di;

	if ((ret = kkt_begin_doc(doc_type, err_info, &err_info_len)) != 0) {
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

				printf("tlv_buf_size = %d\n", tlv_buf_size);

				if ((ret = kkt_send_doc_data(tlv_buf, tlv_buf_size, err_info, &err_info_len)) != 0) {
					printf("kkt_send_doc_data->ret = %.2x\n", ret);

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
			tlv_buf_size += FFD_TLV_SIZE(tlv);
			tlv = FFD_TLV_NEXT(tlv);
		}

	}

   	if ((pattern = load_pattern(doc_type, pattern_footer,
				   	pattern_footer_size, &pattern_size)) == NULL) {
		printf("load_pattern fail\n");
		return -1;
	}

	if ((ret = kkt_end_doc(doc_type, pattern, pattern_size, &di, err_info, &err_info_len)) != 0) {
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

// регистрация/перерегистрация
int fd_registration(fd_registration_params_t *params) {
	int i;
	uint16_t reg_mode_tags[] = { 1056, 1002, 1001, 1109, 1110, 1108 };
	uint8_t doc_type;

	ffd_tlv_reset();

	ffd_tlv_add_string(1048, params->user_name, strlen(params->user_name), false);
	ffd_tlv_add_string(1018, params->user_inn, 12, true);
	ffd_tlv_add_string(1009, params->pay_address, strlen(params->pay_address), false);
	ffd_tlv_add_string(1187, params->pay_place, strlen(params->pay_place), false);
	ffd_tlv_add_uint8(1062, params->tax_systems);
	ffd_tlv_add_string(1037, params->reg_number, 20, true);

	for (i = 0; i < ASIZE(reg_mode_tags); i++)
		if ((params->reg_modes & (1 << i)) != 0)
			ffd_tlv_add_uint8(reg_mode_tags[i], 1);

	if (params->automat_number[0] != 0)
		ffd_tlv_add_string(1036, params->automat_number, strlen(params->automat_number), false);

	ffd_tlv_add_string(1021, params->cashier, strlen(params->cashier), false);
	if (params->cashier_inn[0] != 0)
		ffd_tlv_add_string(1023, params->cashier_inn, 12, true);

	ffd_tlv_add_string(1060, params->tax_service_site, strlen(params->tax_service_site), false);
	ffd_tlv_add_string(1117, params->cheque_sender_email, strlen(params->cheque_sender_email), false);
	ffd_tlv_add_string(1046, params->ofd_name, strlen(params->ofd_name), false);
	ffd_tlv_add_string(1017, params->ofd_inn, 12, true);

	if (params->rereg_reason != 0) {
		doc_type = RE_REGISTRATION;

		for (int i = 0; i < 4; i++) {
			if ((params->rereg_reason & (1 << i)) != 0)
				ffd_tlv_add_uint8(1101, i + 1);
		}
	} else
		doc_type = REGISTRATION;

	return fd_create_doc(doc_type, NULL, 0);
}

// открытие смены
int fd_open_shift(fd_shift_params_t *params)
{
	printf("shift_params:\n\tcashier: %s\n\tcashier_inn: %s\n",
			params->cashier, params->cashier_inn);

	ffd_tlv_reset();
	ffd_tlv_add_string(1021, params->cashier, strlen(params->cashier), false);
	if (params->cashier_inn[0] != 0)
		ffd_tlv_add_string(1023, params->cashier_inn, 12, true);

	return fd_create_doc(OPEN_SHIFT, NULL, 0);
}

// закрытие смены
int fd_close_shift(fd_shift_params_t *params)
{
	ffd_tlv_reset();
	ffd_tlv_add_string(1021, params->cashier, strlen(params->cashier), false);
	if (params->cashier_inn[0] != 0)
		ffd_tlv_add_string(1023, params->cashier_inn, 12, true);

	return fd_create_doc(CLOSE_SHIFT, NULL, 0);
}

// отчет о текущих расчетах
int fd_calc_report() {
	ffd_tlv_reset();
	return fd_create_doc(CALC_REPORT, NULL, 0);
}
