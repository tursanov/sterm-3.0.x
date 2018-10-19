/*
 * Программа удаления лицензии ИПТ с терминала.
 * При удалении в MBR записывается специальный флаг, а пользователю
 * выдается 32-значное шестнадцатеричное число. Число формируется
 * на основе текущей даты, таблицы случайных чисел (rndtbl.c) и
 * хэша заводского номера терминала.
 * (c) gsr 2006
 */

#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "colortty.h"
#include "license.h"
#include "md5.h"
#include "paths.h"
#include "rndtbl.h"
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

/* Запись признака установленной лицензии */
static bool write_lic_signature(int dev)
{
	if (read_mbr(dev)){
		*(uint16_t *)(mbr + LIC_SIGNATURE_OFFSET) = LIC_SIGNATURE;
		return write_mbr(dev);
	}else
		return false;
}

/* Удаление файла банковской лицензии */
static bool rm_lic_file(void)
{
	struct stat st;
	if (stat(BANK_LICENSE, &st) == -1){
		if (errno == ENOENT)
			return true;
		else{
			fprintf(stderr, tRED
				"ошибка получения информации о лицензии ИПТ: %s\n"
				ANSI_DEFAULT, strerror(errno));
			return false;
		}
	}else if (!S_ISREG(st.st_mode)){
		fprintf(stderr, tRED "неверный формат лицензии ИПТ\n" ANSI_DEFAULT);
		return false;
	}else if (unlink(BANK_LICENSE) == -1){
		fprintf(stderr, tRED "ошибка удаления лицензии ИПТ: %s\n"
			ANSI_DEFAULT, strerror(errno));
		return false;
	}else
		return true;
}

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

/* Формирование числа для подтверждения удаления лицензии */
static bool mk_del_hash(uint8_t *buf)
{
	struct md5_hash number;
	uint8_t *p = (uint8_t *)&number;
	uint32_t t;
	int i, index;
	if (!read_term_number(USB_BIND_FILE, &number))
		return false;
	t = (uint32_t)time(NULL);
	index = u_times() % NR_RND_REC;
	buf[0] = t >> 24;
	buf[1] = rnd_tbl[index].a >> 24;
	buf[2] = (rnd_tbl[index].a >> 16) & 0xff;
	buf[3] = (rnd_tbl[index].a >> 8) & 0xff;
	buf[4] = (t >> 16) & 0xff;
	buf[5] = rnd_tbl[index].a & 0xff;
	buf[6] = rnd_tbl[index].b >> 24;
	buf[7] = (rnd_tbl[index].b >> 16) & 0xff;
	buf[8] = (t >> 8) & 0xff;
	buf[9] = (rnd_tbl[index].b >> 8) & 0xff;
	buf[10] = rnd_tbl[index].b & 0xff;
	buf[11] = rnd_tbl[index].c >> 24;
	buf[12] = t & 0xff;
	buf[13] = (rnd_tbl[index].c >> 16) & 0xff;
	buf[14] = (rnd_tbl[index].c >> 8) & 0xff;
	buf[15] = rnd_tbl[index].c & 0xff;
	for (i = 0; i < 16; i++)
		buf[i] ^= p[i];
	for (i = 0; i < 16; i++)
		buf[(i + 1) % 16] ^= buf[i];
	return true;
}

/* Вывод номера на экран */
static void print_number(uint8_t *number)
{
	int i;
	printf(CLR_SCR);
	printf(ANSI_PREFIX "12;H" tCYA
"               Банковская лицензия удалена с вашего терминала.\n"
"   Для подтверждения удаления сообщите, пожалуйста, номер, указанный ниже,\n"
"                             в АО НПЦ \"Спектр\"\n");
	printf(ANSI_PREFIX "16;20H" tYLW);
	for (i = 0; i < 16; i++){
		if ((i > 0) && (i < 15) && ((i % 2) == 0))
			putchar('-');
		printf("%.2hhX", number[i]);
	}
	printf(ANSI_DEFAULT "\n");
}

/* Очистка входного потока */
static void flush_stdin(void)
{
	int flags = fcntl(STDIN_FILENO, F_GETFL);
	if (flags == -1)
		return;
	if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1)
		return;
	while (getchar() != EOF);
	fcntl(STDIN_FILENO, F_SETFL, flags);
}

/* Очистка входного потока и чтение из него символа */
static int flush_getchar(void)
{
	flush_stdin();
	return getchar();
}

/* Запрос подтверждения удаления лицензии */
static bool ask_confirmation(void)
{
	int c;
	while (true){
		printf(CLR_SCR);
		printf(	ANSI_PREFIX "10;H" tYLW
"      Вы действительно хотите удалить лицензию ИПТ с данного терминала ?\n"
			ANSI_PREFIX "11;30H" tGRN "1" tYLW " -- да;\n"
			ANSI_PREFIX "12;30H" tGRN "2" tYLW " -- нет\n"
			ANSI_PREFIX "13;30H" tWHT "Ваш выбор: " ANSI_DEFAULT);
		c = flush_getchar();
		switch (c){
			case '1':
				return true;
			case '2':
				printf(CLR_SCR ANSI_PREFIX "12;23H");
				printf(tRED "Лицензия ИПТ НЕ будет удалена\n" ANSI_DEFAULT);
				return false;
		}
	}
}

int main()
{
	uint8_t buf[16];
	int ret = 1, term_dev;
	if (!ask_confirmation())
		return 1;
	term_dev = open_term_dev();
	if (term_dev != -1){
		if (mk_del_hash(buf) && rm_lic_file() &&
				write_lic_signature(term_dev)){
			print_number(buf);
			ret = 0;
		}
		close(term_dev);
	}
	return ret;
}
