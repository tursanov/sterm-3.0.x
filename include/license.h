/* Работа с лицензиями ИПТ. (c) gsr 2005-2006 */

#if !defined LICENSE_H
#define LICENSE_H

#include "md5.h"

/* Размер сектора жесткого диска */
#define SECTOR_SIZE		512
/* Признак установленной лицензии */
#define LIC_SIGNATURE		0x8680
/* Смещение признака установленной лицензии в MBR */
#define LIC_SIGNATURE_OFFSET	0x1fa
/* Файл устройства для работы с DiskOnChip */
#define DOC_DEV			"/dev/msys/fla"
/* Файл устройства для работы с DiskOnModule */
#define DOM_DEV			"/dev/hda"

/* Максимальное количество лицензий в файле */
#define MAX_LICENSES		2000
/* Имя файла хеша заводского номера терминала */
#define TERM_NUMBER_FILE	"/sdata/disk.dat"

/* Информация о лицензии ИПТ */
struct license_info {
	struct md5_hash number;
	struct md5_hash license;
};

#endif		/* LICENSE_H */
