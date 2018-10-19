/* Создание файла списка лицензий ИПТ. (c) gsr 2005 */

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "base64.h"
#include "license.h"
#include "tki.h"

static struct license_info licenses[MAX_LICENSES];

static bool is_space(uint8_t ch)
{
	return (ch == ' ') || (ch == '\n') || (ch == '\r');
}

static bool is_number_prefix(uint8_t ch)
{
	static char *prefixes = "ADEF";
	int i;
	for (i = 0; prefixes[i]; i++){
		if (ch == prefixes[i])
			return true;
	}
	return false;
}

/*
 * Чтение очередного заводского номера из файла. Возвращает число
 * считанных номеров (0/1) или -1 в случае ошибки.
 */
static int read_number(int fd, uint8_t *number)
{
	enum {
		st_space,
		st_infix,
		st_number,
		st_end,
		st_stop,
		st_err,
	};
	static char *infix = "00137001";
	int st = st_space, n = 0, i = 0;
	uint8_t b;
	while ((st != st_stop) && (st != st_err)){
		if (read(fd, &b, 1) != 1){
			switch (st){
				case st_space:
					return 0;
				case st_end:
					return 1;
				default:
					return -1;
			}
		}
		switch (st){
			case st_space:
				if (is_number_prefix(b)){
					number[i++] = b;
					st = st_infix;
				}else if (!is_space(b))
					st = st_err;
				break;
			case st_infix:
				if (b == infix[n++]){
					number[i++] = b;
					if (!infix[n]){
						n = 4;
						st = st_number;
					}
				}else
					st = st_err;
				break;
			case st_number:
				if (isdigit(b)){
					number[i++] = b;
					if (--n == 0)
						st = st_end;
				}else
					st = st_err;
				break;
			case st_end:
				st = is_space(b) ? st_stop : st_err;
		}
	}
	return st == st_stop ? 1 : -1;
}

/* Создание записи для лицензии ИПТ */
static void make_license_info(uint8_t *number, struct license_info *li)
{
	uint8_t buf[2 * TERM_NUMBER_LEN];
	uint8_t v1[64], v2[64];
	int i, l;
/* Создание хеша заводского номера */
	l = base64_encode(number, TERM_NUMBER_LEN, v1);
	encrypt_data(v1, l);
	get_md5(v1, l, &li->number);
/* Создание хеша лицензии ТКИ */
	memcpy(buf, number, TERM_NUMBER_LEN);
	for (i = 0; i < TERM_NUMBER_LEN; i++)
		buf[sizeof(buf) - i - 1] = ~buf[i];
	l = base64_encode(buf, sizeof(buf), v1);
	l = base64_encode(v1, l, v2);
	get_md5(v2, l, &li->license);
}

/* Создание списка лицензий ИПТ */
static int make_licenses(int fd, struct license_info *licenses)
{
	int n = 0, ret;
	uint8_t number[TERM_NUMBER_LEN];
	while ((ret = read_number(fd, number)) == 1){
		if (n == MAX_LICENSES){
			fprintf(stderr, "будет обработано не более %d номеров\n",
				MAX_LICENSES);
			break;
		}
		make_license_info(number, licenses + n++);
	}
	if (ret == -1){
		fprintf(stderr, "ошибка чтения заводского номера #%d\n", n + 1);
		return -1;
	}else
		return n;
}

/* Вывод списка лицензий в stdout */
static void write_licenses(struct license_info *licenses, int nr_licenses)
{
	write(STDOUT_FILENO, licenses, nr_licenses * sizeof(struct license_info));
}

/* Открытие файла списка номеров */
static int open_number_file(char *name)
{
	struct stat st;
	int fd;
	if (stat(name, &st) == -1){
		fprintf(stderr, "ошибка получения информации о %s: %s\n",
			name, strerror(errno));
		return -1;
	}else if (!S_ISREG(st.st_mode)){
		fprintf(stderr, "файл %s не является обычным файлом\n", name);
		return -1;
	}
	fd = open(name, O_RDONLY);
	if (fd == -1)
		fprintf(stderr, "ошибка открытия %s для чтения: %s\n",
			name, strerror(errno));
	return fd;
}

int main(int argc, char **argv)
{
	int fd, nr_licenses;
	if (argc != 2){
		fprintf(stderr, "укажите имя файла списка номеров\n");
		return 1;
	}
	fd = open_number_file(argv[1]);
	if (fd == -1)
		return 1;
	nr_licenses = make_licenses(fd, licenses);
	if (nr_licenses <= 0)
		return 1;
	write_licenses(licenses, nr_licenses);
	close(fd);
	return 0;
}
