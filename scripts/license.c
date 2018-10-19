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

/* Буфер для работы с MBR */
static uint8_t mbr[SECTOR_SIZE];

/* Чтение MBR заданного устройства */
static bool read_mbr(int dev)
{
	return	(lseek(dev, 0, SEEK_SET) == 0) &&
		(read(dev, mbr, sizeof(mbr)) == sizeof(mbr));
}

#if 0
/* Запись MBR заданного устройства */
static bool write_mbr(int dev)
{
	return	(lseek(dev, 0, SEEK_SET) == 0) &&
		(write(dev, mbr, sizeof(mbr)) == sizeof(mbr));
}
#endif

/* Открытие устройства для чтения/записи LIC_SIGNATURE */
static int open_term_dev(void)
{
	int dev = open(DOM_DEV, O_RDWR);
	if (dev == -1)
		dev = open(DOC_DEV, O_RDWR);
	return dev;
}

/* Проверка писутствия признака установки лицензии */
static bool check_lic_signature(int dev)
{
	if (read_mbr(dev))
		return *(uint16_t *)(mbr + LIC_SIGNATURE_OFFSET) == LIC_SIGNATURE;
	else
		return true;	/* будем считать, что лицензия уже установлена */
}

#if 0
/* Запись признака установленной лицензии */
static bool write_lic_signature(int dev)
{
	if (read_mbr(dev)){
		*(uint16_t *)(mbr + LIC_SIGNATURE_OFFSET) = LIC_SIGNATURE;
		return write_mbr(dev);
	}else
		return false;
}
#endif

static struct license_info licenses[MAX_LICENSES];
static int nr_licenses;

/* Чтение файла лицензий */
static int read_license_file(char *name, struct license_info *licenses)
{
	struct stat st;
	div_t d;
	int fd, ret = -1;
	if (stat(name, &st) == -1){
		fprintf(stderr, "ошибка получения информации о файле %s: %s\n",
			name, strerror(errno));
		return ret;
	}else if (!S_ISREG(st.st_mode)){
		fprintf(stderr, "файл %s не является обычным файлом\n", name);
		return ret;
	}
	d = div(st.st_size, sizeof(struct license_info));
	if ((d.quot == 0) || (d.rem != 0)){
		fprintf(stderr, "файл %s имеет неверный размер\n", name);
		return ret;
	}else if (d.quot > MAX_LICENSES){
		fprintf(stderr, "число лицензий не может превышать %d\n",
			MAX_LICENSES);
		return ret;
	}
	fd = open(name, O_RDONLY);
	if (fd == -1){
		fprintf(stderr, "ошибка открытия %s для чтения: %s\n",
			name, strerror(errno));
		return ret;
	}
	if (read(fd, licenses, st.st_size) == st.st_size)
		ret = d.quot;
	if (ret == -1)
		fprintf(stderr, "ошибка чтения из %s: %s\n",
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
		fprintf(stderr, "ошибка получения информации о файле %s: %s\n",
			name, strerror(errno));
		return false;
	}else if (!S_ISREG(st.st_mode)){
		fprintf(stderr, "файл %s не является обычным файлом\n", name);
		return false;
	}
	if (st.st_size != sizeof(struct md5_hash)){
		fprintf(stderr, "файл %s имеет неверный размер\n", name);
		return false;
	}
	fd = open(name, O_RDONLY);
	if (fd == -1){
		fprintf(stderr, "ошибка открытия %s для чтения: %s\n",
			name, strerror(errno));
		return false;
	}
	ret = read(fd, number, sizeof(*number)) == sizeof(*number);
	if (!ret)
		fprintf(stderr, "ошибка чтения из %s: %s\n",
			name, strerror(errno));
	close(fd);
	return ret;
}

/* Поиск банковской лицензии по хешу заводского номера */
static struct md5_hash *find_license(struct license_info *licenses,
		int nr_licenses, struct md5_hash *number)
{
	int i;
	for (i = 0; i < nr_licenses; i++){
		if (cmp_md5(&licenses[i].number, number))
			return &licenses[i].license;
	}
	return NULL;
}

/* Вывод банковской лицензии в stdout */
static void write_license(struct md5_hash *license)
{
	write(STDOUT_FILENO, license, sizeof(*license));
}

int main(int argc, char **argv)
{
	int dev;
	bool flag;
	struct md5_hash number, *license;
	if (argc != 2){
		fprintf(stderr, "укажите имя файла списка лицензий для работы с ИПТ\n");
		return 1;
	}else if (!read_term_number(TERM_NUMBER_FILE, &number))
		return 1;
	dev = open_term_dev();
	if (dev == -1)
		return 1;
	flag = check_lic_signature(dev);
	close(dev);
	if (flag){
		fprintf(stderr, "установка лицензии для работы с ИПТ на данный терминал невозможна;\n"
				"обратитесь, пожалуйста, в АО НПЦ \"Спектр\".\n");
		return 1;
	}
	nr_licenses = read_license_file(argv[1], licenses);
	if (nr_licenses == -1)
		return 1;
	license = find_license(licenses, nr_licenses, &number);
	if (license == NULL){
		fprintf(stderr, "не найдена лицензия для этого терминала\n");
		return 1;
	}
	write_license(license);
	return 0;
}
