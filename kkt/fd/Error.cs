using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Terminal.KKT.FFD {
	public class Error {
		public static string GetError(byte status, byte[] err_info, int err_info_len) {
			switch (status) {
			case 0x00: // STATUS_OK
				return "нет ошибок";
			case 0x30: // STATUS_OK2
				return "нет ошибок";
			case 0x41: // STATUS_PAPER_END
				return "конец бумаги";
			case 0x42: // STATUS_COVER_OPEN
				return "крышка открыта";
			case 0x43: // STATUS_PAPER_LOCK
				return "бумага застряла на выходе";
			case 0x44: // STATUS_PAPER_WRACK
				return "бумага замялась";
			case 0x45: // STATUS_FS_ERR
				string ex_err;
				if (err_info_len > 0) {
					ex_err = string.Format("(Код команды: {0:X2}h, Код ошибки: {1:X2}h): {2}", err_info[0], err_info[1], GetFNError(err_info[1]));
				} else
					ex_err = "";
				return "общая аппаратная ошибка ФН" + ex_err;
			case 0x46: //
				return "последний сформированный документ не отпечатан";
			case 0x48: // STATUS_CUT_ERR
				return "ошибка отрезки бумаги";
			case 0x49: // STATUS_INIT
				return "ФР находится в состоянии инициализации";
			case 0x4a: // STATUS_THERMHEAD_ERR
				return "неполадки термоголовки";
			case 0x4d: // STATUS_PREV_INCOMPLETE
				return "предыдущая команда была принята не полностью";
			case 0x4e: // STATUS_CRC_ERR
				return "предыдущая команда была принята с ошибкой контрольной суммы";
			case 0x4f: // STATUS_HW_ERR
				return "общая аппаратная ошибка ФР";
			case 0x50: // STATUS_NO_FFEED
				return "нет команды отрезки бланка";
			case 0x51: // STATUS_VPOS_OVER
				return "превышение объёма текста по вертикальным позициям";
			case 0x52: // STATUS_HPOS_OVER
				return "превышение объёма текста по горизонтальным позициям";
			case 0x53: // STATUS_LOG_ERR
				return "нарушение структуры информации при печати КЛ";
			case 0x55: // STATUS_GRID_ERROR
				return "нарушение параметров нанесения макетов";
			case 0x70: // STATUS_BCODE_PARAM
				return "нарушение параметров нанесения штрих-кода";
			case 0x71: // STATUS_NO_ICON
				return "пиктограмма не найдена";
			case 0x72: // STATUS_GRID_WIDTH
				return "ширина сетки больше ширины бланка в установках";
			case 0x73: // STATUS_GRID_HEIGHT
				return "высота сетки больше высоты бланка в установках";
			case 0x74: // STATUS_GRID_NM_FMT
				return "неправильный формат имени сетки";
			case 0x75: // STATUS_GRID_NM_LEN
				return "длина имени сетки больше допустимой";
			case 0x76: // STATUS_GRID_NR
				return "неверный формат номера сетки";
			case 0x77: // STATUS_INVALID_ARG
				return "неверный параметр";
			case 0x80: // STATUS_INVALID_TAG
				if (err_info_len > 0)
					ex_err = GetTagError(err_info, err_info_len);
				else
					ex_err = "";
				return "Ошибка в TLV " + ex_err;
			case 0x81: // STATUS_GET_TIME
				return "Ошибка при получении даты/времени";
			case 0x82: // STATUS_SEND_DOC
				return "Ошибка при передаче данных в ФН";
			case 0x83: // STATUS_TLV_OVERSIZE
				return "Выход за границы общей длины TLV-данных";
			case 0x84: // STATUS_INVALID_STATE
				return "неправильное состояние";
			case 0x86: // STATUS_SHIFT_NOT_CLOSED
				return "Смена не закрыта";
			case 0x87: // STATUS_SHIFT_NOT_OPENED
				return "Смена не открыта";
			case 0x88: // STATUS_NOMEM
				return "Не хватает памяти для завершения операции";
			case 0x89: // STATUS_SAVE_REG_DATA
				return "Ошибка записи регистрационных данных";
			case 0x8a: // STATUS_READ_REG_DATA
				return "Ошибка чтения регистрационных данных";
			case 0x8b: // STATUS_INVALID_INPUT
				return "Неправильный символ в шаблоне";
			case 0x8c: // STATUS_INVALID_TAG_IN_PATTERN
				if (err_info_len > 0)
					ex_err = GetTagError(err_info, err_info_len);
				else
					ex_err = "";
				return "Неправильный тэг в шаблоне " + ex_err;
			case 0x8d: // STATUS_SHIFT_ALREADY_CLOSED
				return "Смена уже закрыта";
			case 0x8e: // STATUS_SHIFT_ALREADY_OPENED
				return "Смена уже открыта";
			case 0x85: // STATUS_FS_REPLACED
				return "Нельзя формировать ФД на другом ФН (допускается формирование ФД только на том ФН, с которым была проведена процедура регистрации (перерегистрации в связи с заменой ФН)";
			case 0xf1: // STATUS_TIMEOUT
				return "Таймаут приема ответа от ФР";
			default:
				return "неизвестная ошибка";
			}
		}

		public static string GetFNError(byte err) {
			switch (err) {
			case 0x00: // FS_RET_SUCCESS
				return "Нет ошибок";
			case 0x01: // FS_RET_UNKNOWN_CMD_OR_ARGS
				return "Неизвестная команда, неверный формат посылки или неизвестные параметры.";
			case 0x02: // FS_RET_INVAL_STATE
				return "Неверное состояние ФН";
			case 0x03: // FS_RET_FS_ERROR
				return "Ошибка ФН";
			case 0x04: // FS_RET_CC_ERROR
				return "Ошибка КС (криптографический сопроцессор)";
			case 0x05: // FS_RET_LIFETIME_OVER
				return "Закончен срок эксплуатации ФН";
			case 0x06: // FS_RET_ARCHIVE_OVERFLOW
				return "Архив ФН переполнен";
			case 0x07: // FS_RET_INVALID_DATETIME
				return "Неверные дата и/или время";
			case 0x08: // FS_RET_NO_DATA
				return "Запрошенные данные отсутствуют в Архиве ФН";
			case 0x09: // FS_RET_INVAL_ARG_VALUE
				return "Параметры команды имеют правильный формат, но их значение не верно";
			case 0x10: // FS_RET_TLV_OVERSIZE
				return "Размер передаваемых TLV данных превысил допустимый";
			case 0x0a: // FS_RET_INVALID_CMD
				return "Некорректная команда.";
			case 0x0b: // FS_RET_ILLEGAL_ATTR
				return "Неразрешенные реквизиты.";
			case 0x0c: // FS_RET_DUP_ATTR
				return "Дублирование данных	ККТ передает данные, которые уже были переданы в составе данного документа.";
			case 0x0d: // FS_RET_MISS_ATTR
				return "Отсутствуют данные, необходимые для корректного учета в ФН.";
			case 0x0e: // FS_RET_POS_OVERFLOW
				return "Количество позиций в документе, превысило допустимый предел.";
			case 0x11: // FS_RET_NO_TRANSPORT
				return "Нет транспортного соединения";
			case 0x12: // FS_RET_CC_OUT
				return "Исчерпан ресурс КС (криптографического сопроцессора)";
			case 0x14: // FS_RET_ARCHIVE_OUT
				return "Исчерпан ресурс хранения";
			case 0x15: // FS_RET_MSG_SEND_TIMEOUT
				return "Исчерпан ресурс Ожидания передачи сообщения";
			case 0x16: // FS_RET_SHIFT_TIMEOUT
				return "Продолжительность смены более 24 часов";
			case 0x17: // FS_RET_WRONG_PERIOD
				return "Неверная разница во времени между 2 операциями";
			case 0x18: // FS_RET_INVALID_ATTR
				return "Некорректный реквизит, переданный ККТ в ФН	Реквизит, переданный ККТ в ФН, не соответствует установленным требованиям.";
			case 0x19: // FS_RET_INVALID_ATTR_EXISABLE
				return "Некорректный реквизит с признаком продажи подакцизного товара.";
			case 0x20: // FS_RET_MSG_NOT_ACCEPTED
				return "Сообщение от ОФД не может быть принято";
			case 0x21: // FS_RET_INVALID_RESP_DATA_SIZE
				return "Длина данных в ответе меньше ожидаемой";
			case 0x22: // FS_RET_INVAL
				return "Неправильное значение данных в команде";
			case 0x23: // FS_RET_TRANSPORT_ERROR
				return "Ошибка при передаче данных в ФН";
			case 0x24: // FS_RET_SEND_TIMEOUT
				return "Таймаут передачи в ФН";
			case 0x25: // FS_RET_RECV_TIMEOUT
				return "Таймайт приема из ФН";
			case 0x26: // FS_RET_CRC_ERROR
				return "Ошибка CRC";
			case 0x27: // FS_RET_WRITE_ERROR
				return "Ошибка записи в ФН";
			case 0x29: // FS_RET_READ_ERROR
				return "Ошибка приема из ФН";
			default:
				return "Неизвестная ошибка";
			}
		}

		static string GetTagError(byte[] err, int err_len) {
			StringBuilder sb = new StringBuilder();
			sb.AppendFormat("[Тэг: {0:D4}] ", BitConverter.ToUInt16(err, 0));
			switch (err[2]) {
			case 0x01: // ERR_TAG_UNKNOWN
			sb.Append("Неизвестный для данного документа TLV");
			break;
			case 0x02: // ERR_TAG_NOT_FOR_INPUT
			sb.Append("Данный TLV заполняется ККТ и не должен передаваться");
			break;
			case 0x03: // ERR_TAG_STLV_FIELDS_NOT_FILLED
			sb.Append("Не все поля в STLV заполнены");
			break;
			case 0x04: // ERR_TAG_EMPTY
			sb.Append("TLV не заполнен (нулевая длина данных)");
			break;
			case 0x05: // ERR_TAG_TOO_LONG
			sb.Append("Длина TLV превышает допустимую");
			break;
			case 0x06: // ERR_TAG_LENGTH
			sb.Append("Неправильная длина TLV (Для данного TLV длина фиксирована)");
			break;
			case 0x07: // ERR_TAG_CONTENT
			sb.Append("Неправильное содержимое TLV");
			break;
			case 0x08: // ERR_TAG_METADATA
			sb.Append("Неправильное содержимое в метаданных");
			break;
			case 0x09: // ERR_TAG_NOT_IN_RANGE
			sb.Append("Значение TLV выходит за разрешенный диапазон");
			break;
			case 0x0a: // ERR_TAG_SAVE
			sb.Append("Ошибка при записи TLV в буфер для отправки в ФН");
			break;
			case 0x0b: // ERR_TAG_EXTRA
			sb.Append("Недопустимый для данного документа TLV");
			break;
			case 0x0c: // ERR_TAG_MISS
			sb.Append("Необходимый для данного документа TLV не был передан");
			break;
			case 0x0d: // ERR_TAG_INVALID_VALUE
			sb.Append("Неправильное значение TLV");
			break;
			case 0x0e: // ERR_TAG_DUPLICATE
			sb.Append("Дублирование тэга, который не может быть повторяющимся");
			break;
			case 0x20: // ERR_TAG_BEGIN_FB
			sb.Append("Ошибка при обработке начала фискального блока в шаблоне для печати");
			break;
			case 0x21: // ERR_TAG_END_FB
			sb.Append("Ошибка при обработке окончания фискального блока в шаблоне для печати");
			break;
			case 0x22: // ERR_TAG_TLV_HDR
			sb.Append("Ошибка при обработке заголовка TLV в шаблоне для печати");
			break;
			case 0x23: // ERR_TAG_TLV_VALUE
			sb.Append("Ошибка при обработке значения TLV в шаблоне для печати");
			break;
			}

			return sb.ToString();
		}
	}
}
