/* Установка на терминал банковской лицензии. (c) gsr 2005 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "license.h"

/* Увеличиваем максимальное количество лицензий */
#if defined MAX_LICENSES
#undef MAX_LICENSES
#endif

#define MAX_LICENSES		10000

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
#if defined __XLICENSE_CHECK_DOC__
	int dev = open(DOC_DEV, O_RDWR);
	if (dev == -1)
		dev = open(DOM_DEV, O_RDWR);
	return dev;
#else
	return open(DOM_DEV, O_RDWR);
#endif		/* __XLICENSE_CHECK_DOC__ */
}

/* Проверка писутствия признака установки лицензии */
static bool check_lic_signature(int dev)
{
	if (read_mbr(dev))
		return *(uint16_t *)(mbr + LIC_SIGNATURE_OFFSET) == LIC_SIGNATURE;
	else
		return true;	/* будем считать, что лицензия уже установлена */
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

static struct license_info licenses[MAX_LICENSES];
static int nr_licenses;

/* Чтение файла лицензий */
static int read_license_file(char *name, struct license_info *licenses)
{
	struct stat st;
	div_t d;
	int fd, ret = -1;
	if (stat(name, &st) == -1){
		fprintf(stderr, "Ошибка получения информации о файле %s: %s.\n",
			name, strerror(errno));
		return ret;
	}else if (!S_ISREG(st.st_mode)){
		fprintf(stderr, "%s не является обычным файлом.\n", name);
		return ret;
	}
	d = div(st.st_size, sizeof(struct license_info));
	if ((d.quot == 0) || (d.rem != 0)){
		fprintf(stderr, "Файл %s имеет неверный размер.\n", name);
		return ret;
	}else if (d.quot > MAX_LICENSES){
		fprintf(stderr, "Число лицензий не может превышать %d.\n",
			MAX_LICENSES);
		return ret;
	}
	fd = open(name, O_RDONLY);
	if (fd == -1){
		fprintf(stderr, "Ошибка открытия %s для чтения: %s.\n",
			name, strerror(errno));
		return ret;
	}
	if (read(fd, licenses, st.st_size) == st.st_size)
		ret = d.quot;
	if (ret == -1)
		fprintf(stderr, "Ошибка чтения из %s: %s.\n",
			name, strerror(errno));
	close(fd);
	return ret;
}

/* Чтение хеша заводского номера терминала */
static bool read_term_number(char *name, struct md5_hash *number)
{
	struct stat st;
	int fd;
	bool ret;
	if (stat(name, &st) == -1){
		fprintf(stderr, "Ошибка получения информации о файле %s: %s.\n",
			name, strerror(errno));
		return false;
	}else if (!S_ISREG(st.st_mode)){
		fprintf(stderr, "%s не является обычным файлом.\n", name);
		return false;
	}
	if (st.st_size != sizeof(struct md5_hash)){
		fprintf(stderr, "Файл %s имеет неверный размер.\n", name);
		return false;
	}
	fd = open(name, O_RDONLY);
	if (fd == -1){
		fprintf(stderr, "Ошибка открытия %s для чтения: %s.\n",
			name, strerror(errno));
		return false;
	}
	ret = read(fd, number, sizeof(*number)) == sizeof(*number);
	if (!ret)
		fprintf(stderr, "Ошибка чтения из %s: %s.\n",
			name, strerror(errno));
	close(fd);
	return ret;
}

/*
 * Поиск банковской лицензии по хешу заводского номера. Если хеш заводского
 * номера повторяется в списке более одного раза, значит лицензия для этого
 * терминала была удалена и мы должны записать в MBR признак удаления
 * лицензии.
 */
static struct md5_hash *find_license(int dev, struct license_info *licenses,
		int nr_licenses, struct md5_hash *number)
{
	int i, n, m;
	for (i = n = m = 0; i < nr_licenses; i++){
		if (cmp_md5(&licenses[i].number, number)){
			n++;
			m = i;
		}
	}
	if (n == 1)
		return &licenses[m].license;
	else{
		if (n > 1)
			write_lic_signature(dev);
		return NULL;
	}
}

/* Вывод банковской лицензии в stdout */
static void write_license(struct md5_hash *license)
{
	write(STDOUT_FILENO, license, sizeof(*license));
}

int main(int argc, char **argv)
{
	int ret = 1, dev;
	struct md5_hash number, *license;
	if (argc != 2)
		fprintf(stderr, "Укажите имя файла списка лицензий для работы с ИПТ.\n");
	else if (!read_term_number(TERM_NUMBER_FILE, &number))
		fprintf(stderr, "Не найден модуль безопасности.\n");
	else{
		dev = open_term_dev();
		if (dev != -1){
			if (check_lic_signature(dev))
				fprintf(stderr, "Установка лицензии для работы "
					"с ИПТ на данный терминал невозможна;\n"
					"обратитесь, пожалуйста, в АО НПЦ \"Спектр\".\n");
			else{
				nr_licenses = read_license_file(argv[1], licenses);
				if (nr_licenses > 0){
					license = find_license(dev, licenses,
						nr_licenses, &number);
					if (license != NULL){
						write_license(license);
						ret = 0;
					}else
						fprintf(stderr, "Не найдена "
							"лицензия для этого терминала.\n");
				}
			}
			close(dev);
		}
	}
	return ret;
}
