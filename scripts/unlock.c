/*
 * Программа разблокирования терминала после удаления с него лицензии ИПТ.
 * После запуска этой программы на терминал снова можно будет установить
 * лицензию ИПТ. (c) gsr 2006
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "colortty.h"
#include "license.h"
#include "md5.h"
#include "tki.h"

/* Буфер для работы с MBR */
static uint8_t mbr[SECTOR_SIZE];

/* Чтение MBR заданного устройства */
static bool read_mbr(int dev)
{
	return	(lseek(dev, 0, SEEK_SET) == 0) &&
		(read(dev, mbr, sizeof(mbr)) == sizeof(mbr));
}

/* Запись MBR заданного устройства */
static bool write_mbr(int dev)
{
	return	(lseek(dev, 0, SEEK_SET) == 0) &&
		(write(dev, mbr, sizeof(mbr)) == sizeof(mbr));
}

/* Открытие устройства для чтения/записи LIC_SIGNATURE */
static int open_term_dev(void)
{
	int dev = open(DOM_DEV, O_RDWR);
	if (dev == -1)
		dev = open(DOC_DEV, O_RDWR);
	if (dev == -1)
		fprintf(stderr, tRED "ошибка открытия носителя терминала\n"
			ANSI_DEFAULT);
	return dev;
}

/* Удаление признака установленной лицензии */
static bool clear_lic_signature(int dev)
{
	if (read_mbr(dev)){
		*(uint16_t *)(mbr + LIC_SIGNATURE_OFFSET) = 0;
		return write_mbr(dev);
	}else
		return false;
}

/* Список заводских номеров для восстановления лицензии */
static struct fuzzy_md5 {
	uint32_t a;
	uint32_t aa;
	uint32_t b;
	uint32_t bb;
	uint32_t c;
	uint32_t cc;
	uint32_t d;
	uint32_t dd;
} known_numbers[] = {
	/* D001370015509 */
	{
		.a = 0xa4d764d7, .aa = 0x312e157f,
		.b = 0xbce002bf, .bb = 0x28a9431e,
		.c = 0xdd8858d8, .cc = 0x84ef3a05,
		.d = 0x8f2f951b, .dd = 0x761aa13d,
	},
};

/* Чтение хеша заводского номера терминала */
static bool read_term_number(char *name, struct md5_hash *number)
{
	struct stat st;
	int fd;
	bool ret;
	if (stat(name, &st) == -1){
		fprintf(stderr, tRED "ошибка получения информации о файле %s: %s\n"
			ANSI_DEFAULT, name, strerror(errno));
		return false;
	}else if (!S_ISREG(st.st_mode)){
		fprintf(stderr, tRED "файл %s не является обычным файлом\n"
			ANSI_DEFAULT, name);
		return false;
	}
	if (st.st_size != sizeof(struct md5_hash)){
		fprintf(stderr, tRED "файл %s имеет неверный размер\n"
			ANSI_DEFAULT, name);
		return false;
	}
	fd = open(name, O_RDONLY);
	if (fd == -1){
		fprintf(stderr, tRED "ошибка открытия %s для чтения: %s\n"
			ANSI_DEFAULT, name, strerror(errno));
		return false;
	}
	ret = read(fd, number, sizeof(*number)) == sizeof(*number);
	if (!ret)
		fprintf(stderr, tRED "ошибка чтения из %s: %s\n"
			ANSI_DEFAULT, name, strerror(errno));
	close(fd);
	return ret;
}

/* Поиск заданного заводского номера в списке */
static bool check_term_number(struct md5_hash *number)
{
	struct fuzzy_md5 *p;
	int i;
	for (i = 0; i < ASIZE(known_numbers); i++){
		p = known_numbers + i;
		if ((p->a == number->a) && (p->b == number->b) &&
				(p->c == number->c) && (p->d == number->d))
			return true;
	}
	return false;
}

/* Вывод на экран сообщения об успешной разблокировке терминала */
static void print_unlock_msg(void)
{
	printf(CLR_SCR);
	printf(ANSI_PREFIX "10;H" tWHT bBlu
"                                                                               \n"
"  ╔═════════════════════════════════════════════════════════════════════════╗  \n"
"  ║" tGRN
   "                        Терминал разблокирован.                          "
tWHT "║  \n"
"  ║" tCYA
   "        Теперь вы можете установить на этот терминал лицензию ИПТ        "
tWHT "║  \n"
"  ║" tCYA
   "                            обычным образом.                             "
tWHT "║  \n"
"  ╚═════════════════════════════════════════════════════════════════════════╝  \n"
"                                                                               \n"
ANSI_DEFAULT);
}

int main()
{
	struct md5_hash number;
	int ret = 1, term_dev;
	term_dev = open_term_dev();
	if (term_dev != -1){
		if (read_term_number(USB_BIND_FILE, &number)){
			if (check_term_number(&number)){
				if (clear_lic_signature(term_dev)){
					print_unlock_msg();
					ret = 0;
				}else
					fprintf(stderr, tRED "ошибка разблокировки терминала\n" ANSI_DEFAULT);
			}else
				fprintf(stderr, tRED "данный терминал не может быть разблокирован этой программой\n" ANSI_DEFAULT);
		}
	}
	return ret;
}
