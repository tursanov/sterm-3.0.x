using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Terminal.KKT.FFD {
	public class Error {
		public static string GetError(byte status, byte[] err_info, int err_info_len) {
			switch (status) {
			case 0x00: // STATUS_OK
				return "��� �訡��";
			case 0x30: // STATUS_OK2
				return "��� �訡��";
			case 0x41: // STATUS_PAPER_END
				return "����� �㬠��";
			case 0x42: // STATUS_COVER_OPEN
				return "���誠 �����";
			case 0x43: // STATUS_PAPER_LOCK
				return "�㬠�� �����﫠 �� ��室�";
			case 0x44: // STATUS_PAPER_WRACK
				return "�㬠�� ���﫠��";
			case 0x45: // STATUS_FS_ERR
				string ex_err;
				if (err_info_len > 0) {
					ex_err = string.Format("(��� �������: {0:X2}h, ��� �訡��: {1:X2}h): {2}", err_info[0], err_info[1], GetFNError(err_info[1]));
				} else
					ex_err = "";
				return "���� �����⭠� �訡�� ��" + ex_err;
			case 0x46: //
				return "��᫥���� ��ନ஢���� ���㬥�� �� �⯥�⠭";
			case 0x48: // STATUS_CUT_ERR
				return "�訡�� ��१�� �㬠��";
			case 0x49: // STATUS_INIT
				return "�� ��室���� � ���ﭨ� ���樠����樨";
			case 0x4a: // STATUS_THERMHEAD_ERR
				return "��������� �ମ�������";
			case 0x4d: // STATUS_PREV_INCOMPLETE
				return "�।���� ������� �뫠 �ਭ�� �� ���������";
			case 0x4e: // STATUS_CRC_ERR
				return "�।���� ������� �뫠 �ਭ�� � �訡��� ����஫쭮� �㬬�";
			case 0x4f: // STATUS_HW_ERR
				return "���� �����⭠� �訡�� ��";
			case 0x50: // STATUS_NO_FFEED
				return "��� ������� ��१�� ������";
			case 0x51: // STATUS_VPOS_OVER
				return "�ॢ�襭�� ���� ⥪�� �� ���⨪���� ������";
			case 0x52: // STATUS_HPOS_OVER
				return "�ॢ�襭�� ���� ⥪�� �� ��ਧ��⠫�� ������";
			case 0x53: // STATUS_LOG_ERR
				return "����襭�� �������� ���ଠ樨 �� ���� ��";
			case 0x55: // STATUS_GRID_ERROR
				return "����襭�� ��ࠬ��஢ ����ᥭ�� ����⮢";
			case 0x70: // STATUS_BCODE_PARAM
				return "����襭�� ��ࠬ��஢ ����ᥭ�� ����-����";
			case 0x71: // STATUS_NO_ICON
				return "���⮣ࠬ�� �� �������";
			case 0x72: // STATUS_GRID_WIDTH
				return "�ਭ� �⪨ ����� �ਭ� ������ � ��⠭�����";
			case 0x73: // STATUS_GRID_HEIGHT
				return "���� �⪨ ����� ����� ������ � ��⠭�����";
			case 0x74: // STATUS_GRID_NM_FMT
				return "���ࠢ���� �ଠ� ����� �⪨";
			case 0x75: // STATUS_GRID_NM_LEN
				return "����� ����� �⪨ ����� �����⨬��";
			case 0x76: // STATUS_GRID_NR
				return "������ �ଠ� ����� �⪨";
			case 0x77: // STATUS_INVALID_ARG
				return "������ ��ࠬ���";
			case 0x80: // STATUS_INVALID_TAG
				if (err_info_len > 0)
					ex_err = GetTagError(err_info, err_info_len);
				else
					ex_err = "";
				return "�訡�� � TLV " + ex_err;
			case 0x81: // STATUS_GET_TIME
				return "�訡�� �� ����祭�� ����/�६���";
			case 0x82: // STATUS_SEND_DOC
				return "�訡�� �� ��।�� ������ � ��";
			case 0x83: // STATUS_TLV_OVERSIZE
				return "��室 �� �࠭��� ��饩 ����� TLV-������";
			case 0x84: // STATUS_INVALID_STATE
				return "���ࠢ��쭮� ���ﭨ�";
			case 0x86: // STATUS_SHIFT_NOT_CLOSED
				return "����� �� ������";
			case 0x87: // STATUS_SHIFT_NOT_OPENED
				return "����� �� �����";
			case 0x88: // STATUS_NOMEM
				return "�� 墠⠥� ����� ��� �����襭�� ����樨";
			case 0x89: // STATUS_SAVE_REG_DATA
				return "�訡�� ����� ॣ����樮���� ������";
			case 0x8a: // STATUS_READ_REG_DATA
				return "�訡�� �⥭�� ॣ����樮���� ������";
			case 0x8b: // STATUS_INVALID_INPUT
				return "���ࠢ���� ᨬ��� � 蠡����";
			case 0x8c: // STATUS_INVALID_TAG_IN_PATTERN
				if (err_info_len > 0)
					ex_err = GetTagError(err_info, err_info_len);
				else
					ex_err = "";
				return "���ࠢ���� �� � 蠡���� " + ex_err;
			case 0x8d: // STATUS_SHIFT_ALREADY_CLOSED
				return "����� 㦥 ������";
			case 0x8e: // STATUS_SHIFT_ALREADY_OPENED
				return "����� 㦥 �����";
			case 0x85: // STATUS_FS_REPLACED
				return "����� �ନ஢��� �� �� ��㣮� �� (����᪠���� �ନ஢���� �� ⮫쪮 �� ⮬ ��, � ����� �뫠 �஢����� ��楤�� ॣ����樨 (���ॣ����樨 � �裡 � ������� ��)";
			case 0xf1: // STATUS_TIMEOUT
				return "������� �ਥ�� �⢥� �� ��";
			default:
				return "�������⭠� �訡��";
			}
		}

		public static string GetFNError(byte err) {
			switch (err) {
			case 0x00: // FS_RET_SUCCESS
				return "��� �訡��";
			case 0x01: // FS_RET_UNKNOWN_CMD_OR_ARGS
				return "�������⭠� �������, ������ �ଠ� ���뫪� ��� ��������� ��ࠬ����.";
			case 0x02: // FS_RET_INVAL_STATE
				return "����୮� ���ﭨ� ��";
			case 0x03: // FS_RET_FS_ERROR
				return "�訡�� ��";
			case 0x04: // FS_RET_CC_ERROR
				return "�訡�� �� (�ਯ⮣���᪨� ᮯ�����)";
			case 0x05: // FS_RET_LIFETIME_OVER
				return "�����祭 �ப �ᯫ��樨 ��";
			case 0x06: // FS_RET_ARCHIVE_OVERFLOW
				return "��娢 �� ��९�����";
			case 0x07: // FS_RET_INVALID_DATETIME
				return "������ ��� �/��� �६�";
			case 0x08: // FS_RET_NO_DATA
				return "����襭�� ����� ���������� � ��娢� ��";
			case 0x09: // FS_RET_INVAL_ARG_VALUE
				return "��ࠬ���� ������� ����� �ࠢ���� �ଠ�, �� �� ���祭�� �� ��୮";
			case 0x10: // FS_RET_TLV_OVERSIZE
				return "������ ��।������� TLV ������ �ॢ�ᨫ �����⨬�";
			case 0x0a: // FS_RET_INVALID_CMD
				return "�����४⭠� �������.";
			case 0x0b: // FS_RET_ILLEGAL_ATTR
				return "��ࠧ�襭�� ४������.";
			case 0x0c: // FS_RET_DUP_ATTR
				return "�㡫�஢���� ������	��� ��।��� �����, ����� 㦥 �뫨 ��।��� � ��⠢� ������� ���㬥��.";
			case 0x0d: // FS_RET_MISS_ATTR
				return "���������� �����, ����室��� ��� ���४⭮�� ��� � ��.";
			case 0x0e: // FS_RET_POS_OVERFLOW
				return "������⢮ ����権 � ���㬥��, �ॢ�ᨫ� �����⨬� �।��.";
			case 0x11: // FS_RET_NO_TRANSPORT
				return "��� �࠭ᯮ�⭮�� ᮥ�������";
			case 0x12: // FS_RET_CC_OUT
				return "���௠� ����� �� (�ਯ⮣���᪮�� ᮯ�����)";
			case 0x14: // FS_RET_ARCHIVE_OUT
				return "���௠� ����� �࠭����";
			case 0x15: // FS_RET_MSG_SEND_TIMEOUT
				return "���௠� ����� �������� ��।�� ᮮ�饭��";
			case 0x16: // FS_RET_SHIFT_TIMEOUT
				return "�த����⥫쭮��� ᬥ�� ����� 24 �ᮢ";
			case 0x17: // FS_RET_WRONG_PERIOD
				return "����ୠ� ࠧ��� �� �६��� ����� 2 �����ﬨ";
			case 0x18: // FS_RET_INVALID_ATTR
				return "�����४�� ४�����, ��।���� ��� � ��	��������, ��।���� ��� � ��, �� ᮮ⢥����� ��⠭������� �ॡ������.";
			case 0x19: // FS_RET_INVALID_ATTR_EXISABLE
				return "�����४�� ४����� � �ਧ����� �த��� �����樧���� ⮢��.";
			case 0x20: // FS_RET_MSG_NOT_ACCEPTED
				return "����饭�� �� ��� �� ����� ���� �ਭ��";
			case 0x21: // FS_RET_INVALID_RESP_DATA_SIZE
				return "����� ������ � �⢥� ����� ���������";
			case 0x22: // FS_RET_INVAL
				return "���ࠢ��쭮� ���祭�� ������ � �������";
			case 0x23: // FS_RET_TRANSPORT_ERROR
				return "�訡�� �� ��।�� ������ � ��";
			case 0x24: // FS_RET_SEND_TIMEOUT
				return "������� ��।�� � ��";
			case 0x25: // FS_RET_RECV_TIMEOUT
				return "������� �ਥ�� �� ��";
			case 0x26: // FS_RET_CRC_ERROR
				return "�訡�� CRC";
			case 0x27: // FS_RET_WRITE_ERROR
				return "�訡�� ����� � ��";
			case 0x29: // FS_RET_READ_ERROR
				return "�訡�� �ਥ�� �� ��";
			default:
				return "�������⭠� �訡��";
			}
		}

		static string GetTagError(byte[] err, int err_len) {
			StringBuilder sb = new StringBuilder();
			sb.AppendFormat("[��: {0:D4}] ", BitConverter.ToUInt16(err, 0));
			switch (err[2]) {
			case 0x01: // ERR_TAG_UNKNOWN
			sb.Append("��������� ��� ������� ���㬥�� TLV");
			break;
			case 0x02: // ERR_TAG_NOT_FOR_INPUT
			sb.Append("����� TLV ���������� ��� � �� ������ ��।�������");
			break;
			case 0x03: // ERR_TAG_STLV_FIELDS_NOT_FILLED
			sb.Append("�� �� ���� � STLV ���������");
			break;
			case 0x04: // ERR_TAG_EMPTY
			sb.Append("TLV �� �������� (�㫥��� ����� ������)");
			break;
			case 0x05: // ERR_TAG_TOO_LONG
			sb.Append("����� TLV �ॢ�蠥� �����⨬��");
			break;
			case 0x06: // ERR_TAG_LENGTH
			sb.Append("���ࠢ��쭠� ����� TLV (��� ������� TLV ����� 䨪�஢���)");
			break;
			case 0x07: // ERR_TAG_CONTENT
			sb.Append("���ࠢ��쭮� ᮤ�ন��� TLV");
			break;
			case 0x08: // ERR_TAG_METADATA
			sb.Append("���ࠢ��쭮� ᮤ�ন��� � ��⠤�����");
			break;
			case 0x09: // ERR_TAG_NOT_IN_RANGE
			sb.Append("���祭�� TLV ��室�� �� ࠧ�襭�� ��������");
			break;
			case 0x0a: // ERR_TAG_SAVE
			sb.Append("�訡�� �� ����� TLV � ���� ��� ��ࠢ�� � ��");
			break;
			case 0x0b: // ERR_TAG_EXTRA
			sb.Append("�������⨬� ��� ������� ���㬥�� TLV");
			break;
			case 0x0c: // ERR_TAG_MISS
			sb.Append("����室��� ��� ������� ���㬥�� TLV �� �� ��।��");
			break;
			case 0x0d: // ERR_TAG_INVALID_VALUE
			sb.Append("���ࠢ��쭮� ���祭�� TLV");
			break;
			case 0x0e: // ERR_TAG_DUPLICATE
			sb.Append("�㡫�஢���� ��, ����� �� ����� ���� �������騬��");
			break;
			case 0x20: // ERR_TAG_BEGIN_FB
			sb.Append("�訡�� �� ��ࠡ�⪥ ��砫� �᪠�쭮�� ����� � 蠡���� ��� ����");
			break;
			case 0x21: // ERR_TAG_END_FB
			sb.Append("�訡�� �� ��ࠡ�⪥ ����砭�� �᪠�쭮�� ����� � 蠡���� ��� ����");
			break;
			case 0x22: // ERR_TAG_TLV_HDR
			sb.Append("�訡�� �� ��ࠡ�⪥ ��������� TLV � 蠡���� ��� ����");
			break;
			case 0x23: // ERR_TAG_TLV_VALUE
			sb.Append("�訡�� �� ��ࠡ�⪥ ���祭�� TLV � 蠡���� ��� ����");
			break;
			}

			return sb.ToString();
		}
	}
}
