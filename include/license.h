/* ����� � ��業��ﬨ ���. (c) gsr 2005-2006 */

#if !defined LICENSE_H
#define LICENSE_H

#include "md5.h"

/* ������ ᥪ�� ���⪮�� ��᪠ */
#define SECTOR_SIZE		512
/* �ਧ��� ��⠭�������� ��業��� */
#define LIC_SIGNATURE		0x8680
/* ���饭�� �ਧ���� ��⠭�������� ��業��� � MBR */
#define LIC_SIGNATURE_OFFSET	0x1fa
/* ���� ���ன�⢠ ��� ࠡ��� � DiskOnChip */
#define DOC_DEV			"/dev/msys/fla"
/* ���� ���ன�⢠ ��� ࠡ��� � DiskOnModule */
#define DOM_DEV			"/dev/hda"

/* ���ᨬ��쭮� ������⢮ ��業��� � 䠩�� */
#define MAX_LICENSES		2000
/* ��� 䠩�� �� �����᪮�� ����� �ନ���� */
#define TERM_NUMBER_FILE	"/sdata/disk.dat"

/* ���ଠ�� � ��業��� ��� */
struct license_info {
	struct md5_hash number;
	struct md5_hash license;
};

#endif		/* LICENSE_H */
