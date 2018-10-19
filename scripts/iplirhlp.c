/*
 * Утилита для записи обновленных ключевых дистрибутивов в модуль
 * безопасности. (c) gsr 2007.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "usb/core.h"
#include "usb/key.h"
#include "iplirhlp.h"
#include "md5.h"
#include "tki.h"

/* Программа работает до тех пор, пока этот флаг установлен */
static bool loop_flag = true;

/* Буфер для записи в модуль безопасности */
static uint8_t key_buf[USB_KEY_SIZE];
/* Буфер для чтения модуля безопасности */
static uint8_t key_rx_buf[USB_KEY_SIZE];

/* Вывод отладочной информации в журнал */
/* Имя файла журнала */
#define LOG_NAME	"/dev/tty5"

/* Дескриптор файла журнала */
static FILE *log_fd = NULL;

/* Закрытие файла журнала */
static void log_close(void)
{
	if (log_fd != NULL){
		fclose(log_fd);
		log_fd = NULL;
	}
}

/* Открытие файла журнала */
static bool log_open(void)
{
	log_close();
	log_fd = fopen(LOG_NAME, "w");
	return log_fd != NULL;
}

/* Запись в журнал времени прихода сообщения */
static bool log_time(void)
{
	if (log_fd != NULL){
		time_t t = time(NULL);
		struct tm *tm = localtime(&t);
		fprintf(log_fd, "%.2d:%.2d:%.2d  ",
			tm->tm_hour, tm->tm_min, tm->tm_sec);
		return true;
	}else
		return false;
}

/* Запись сообщения в журнал */
static bool _log(char *fmt, ...) __attribute__((__format__(printf, 1, 2)));
static bool _log(char *fmt, ...)
{
	va_list p;
	if (log_fd != NULL){
/*
 * Если в начале строки есть символы перевода строки или возврата каретки,
 * печатаем их перед временем поступления сообщения.
 */
		int n = strspn(fmt, "\n\r");
		if (n > 0){
			fprintf(log_fd, "%.*s", n, fmt);
			fmt += n;
		}
		if (log_time()){
			va_start(p, fmt);
			vfprintf(log_fd, fmt, p);
			va_end(p);
		}
		return true;
	}else
		return false;
}

/* Создание файла с идентификатором текущего процесса */
static bool create_pid_file(char *name)
{
	bool ret = false;
	int fd, l;
	char pid[16];
/* Сначала удаляем старый файл, если он есть */
	if (name == NULL)
		return false;
	unlink(name);
/* После этого создаем новый */
	fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
	if (fd == -1)
		_log("Error creating %s: %s.\n", name, strerror(errno));
	else{
		snprintf(pid, sizeof(pid), "%u\n", getpid());
		l = strlen(pid);
		if (write(fd, pid, l) != l)
			_log("Error writing %s: %s.\n", name, strerror(errno));
		else
			ret = true;
		close(fd);
	}
	return ret;
}

/*
 * Поиск в каталоге NEW_DST_DIR_NAME файла с расширением dst. Если файл
 * найден и он один, возвращает имя файла. Иначе возвращает NULL.
 */
static char *scan_new_dst_dir(void)
{
	int selector(const struct dirent *entry)
	{
		char name[64];
		struct stat st;
		int l = strlen(entry->d_name);
		if ((l < 4) || (strcmp(entry->d_name + l - 4, ".dst") != 0))
			return 0;
		snprintf(name, sizeof(name), NEW_DST_DIR_NAME "%s", entry->d_name);
		if (stat(name, &st) == -1)
			return 0;
		else
			return S_ISREG(st.st_mode);
	}
	char *ret = NULL;
	struct dirent **names;
	static char dst_name[64];
	int i, n = scandir(NEW_DST_DIR_NAME, &names, selector, alphasort);
	if (n == -1)
		_log("Failed to scan " NEW_DST_DIR_NAME ": %s.\n",
			strerror(errno));
	else if (n != 1)
		_log("There must be one and only *.dst file in " NEW_DST_DIR_NAME ".\n");
	else{
		snprintf(dst_name, sizeof(dst_name), NEW_DST_DIR_NAME "%s", names[0]->d_name);
		ret = dst_name;
	}
	if (n > 0){
		for (i = 0; i < n; i++)
			free(names[i]);
		free(names);
	}
	return ret;
}

/*
 * Переименование файлов в каталоге IPLIR_UPGRD. Возвращает количество
 * успешно переименованных файлов.
 */
static int rename_upgrd_files(bool ok)
{
	int selector(const struct dirent *entry)
	{
		char name[64];
		struct stat st;
		int l = strlen(entry->d_name);
		if ((l < 5) || (strcmp(entry->d_name + l - 5, ".up1a") != 0))
			return 0;
		snprintf(name, sizeof(name), IPLIR_UPGRD "%s", entry->d_name);
		if (stat(name, &st) == -1)
			return 0;
		else
			return S_ISREG(st.st_mode);
	}
	struct dirent **names;
	char src[64], dst[64], *p, *ext = ok ? "up2" : "up0";
	int ext_len = strlen(ext), ret = 0, i;
	int n = scandir(IPLIR_UPGRD, &names, selector, alphasort);
	if (n == -1)
		_log("Failed to scan " IPLIR_UPGRD ": %s.\n",
			strerror(errno));
	else if (n > 0){
		for (i = 0; i < n; i++){
			snprintf(src, sizeof(src), IPLIR_UPGRD "%s", names[i]->d_name);
			p = strrchr(src, '.');
			if (p != NULL){
				int l = p - src + 1;
				if ((l + ext_len) < sizeof(dst)){
					strncpy(dst, src, l);
					strcpy(dst + l, ext);
					if (rename(src, dst) == 0)
						ret++;
					else
						_log("Failed to rename %s to %s: %s.\n",
							src, dst, strerror(errno));
				}else
					_log("Name %s is too long.\n", src);
			}else
				_log("Name " IPLIR_UPGRD "%s is too long.\n",
					names[i]->d_name);
		}
		for (i = 0; i < n; i++)
			free(names[i]);
		free(names);
	}
	return ret;
}

/* Проверка файла */
static bool check_file(char *name, int exp_len, int max_len)
{
	struct stat st;
	bool ret = false;
	if ((name == NULL) || (exp_len < -1))
		return false;
	else if (stat(USB_BIND_FILE, &st) == -1)
		_log("stat() for %s failed: %s.\n",
			name, strerror(errno));
	else if (!S_ISREG(st.st_mode))
		_log("Wrong type of %s.\n", name);
	else if ((exp_len != -1) && (st.st_size != exp_len))
		_log("File %s has illegal size.\n", name);
	else if (st.st_size > max_len)
		_log("File %s is too long.\n", name);
	else
		ret = true;
	return ret;
}

/*
 * Чтение данных из файла. Возвращает размер считанных данных
 * или -1 в случае ошибки.
 */
static int read_file_data(char *name, struct key_data_header *data_hdr)
{
	int fd, ret = -1;
	uint8_t *data;
	struct stat st;
	if ((name == NULL) || (data_hdr == NULL))
		return -1;
	data = (uint8_t *)(data_hdr + 1);
	if (stat(name, &st) == -1)
		_log("stat() for %s failed: %s\n",
			name, strerror(errno));
	else{
		data_hdr->len = st.st_size + sizeof(*data_hdr);
		get_md5_file(name, &data_hdr->crc);
		fd = open(name, O_RDONLY);
		if (fd == -1)
			_log("Error open %s for reading: %s.\n",
				name, strerror(errno));
		else{
			if (read(fd, data, st.st_size) != st.st_size)
				_log("Error reading %s: %s.\n",
					name, strerror(errno));
			else
				ret = st.st_size;
			close(fd);
		}
	}
	return ret;
}

/* Создание файла привязки ключевого дистрибутива */
static bool create_iplir_bind(char *usb_bind_name,
		char *iplir_name, char *bind_name)
{
	bool ret = false;
	int fd;
	struct md5_hash buf[2], md5;
	if (!get_md5_file(iplir_name, buf))
		_log("Error detecting MD5 sum for %s.\n", iplir_name);
	else{
		fd = open(usb_bind_name, O_RDONLY);
		if (fd == 1)
			_log("Error open %s for reading: %s.\n",
				usb_bind_name, strerror(errno));
		else if (read(fd, buf + 1, sizeof(struct md5_hash)) != sizeof(struct md5_hash)){
			close(fd);
			_log("Error reading %s: %s.\n",
				usb_bind_name, strerror(errno));
		}else{
			close(fd);
			fd = open(bind_name, O_WRONLY | O_TRUNC | O_CREAT, 0600);
			if (fd == -1)
				_log("Failed to create %s: %s.\n",
					bind_name, strerror(errno));
			else{
				get_md5((uint8_t *)buf, sizeof(buf), &md5);
				if (write(fd, &md5, sizeof(md5)) == sizeof(md5))
					ret = true;
				else
					_log("Error writing %s: %s.\n",
						bind_name, strerror(errno));
				close(fd);
			}
		}
	}
	return ret;
}

/* Подготовка буфера для записи в модуль безопасности */
static bool make_key_buf(void)
{
	struct key_header *key_hdr = (struct key_header *)key_buf;
	struct key_data_header *data_hdr =
		(struct key_data_header *)(key_hdr + 1);
	int l, remain = sizeof(key_buf) - sizeof(*key_hdr) - sizeof(*data_hdr);
	memset(key_buf, 0, sizeof(key_buf));
/* Файл привязки */
	if (!check_file(USB_BIND_FILE, sizeof(struct md5_hash), remain) ||
			(read_file_data(USB_BIND_FILE, data_hdr) == -1))
		return false;
/* Файл привязки ключевого дистрибутива */
	if (!create_iplir_bind(USB_BIND_FILE, IPLIR_DST, IPLIR_BIND_FILE))
		return false;
	data_hdr = (struct key_data_header *)((uint8_t *)(data_hdr + 1) +
		sizeof(struct md5_hash));
	remain -= sizeof(*data_hdr) + sizeof(struct md5_hash);
	if (!check_file(IPLIR_BIND_FILE, sizeof(struct md5_hash), remain) ||
			(read_file_data(IPLIR_BIND_FILE, data_hdr) == -1))
		return false;
/* Файл пароля ключевого дистрибутива */
	data_hdr = (struct key_data_header *)((uint8_t *)(data_hdr + 1) +
		sizeof(struct md5_hash));
	remain -= sizeof(*data_hdr) + sizeof(struct md5_hash);
	if (!check_file(IPLIR_PSW_DATA, -1, remain) ||
			((l = read_file_data(IPLIR_PSW_DATA, data_hdr)) == -1))
		return false;
/* Файл ключевого дистрибутива */
	data_hdr = (struct key_data_header *)((uint8_t *)(data_hdr + 1) + l);
	remain -= sizeof(*data_hdr) + l;
	if (!check_file(IPLIR_DST, -1, remain) ||
			((l = read_file_data(IPLIR_DST, data_hdr)) == -1))
		return false;
/* Заголовок модуля */
	data_hdr = (struct key_data_header *)((uint8_t *)(data_hdr + 1) + l);
	key_hdr->magic = USB_KEY_MAGIC;
	key_hdr->data_size = (uint8_t *)(data_hdr + 1) + l - key_buf;
	key_hdr->n_chunks = 4;
	return true;
}

/* Количество попыток записи в модуль безопасности */
#define NR_WTRIES	3
/*
 * Запись данных в модуль безопасности.
 * NB: дополнительная информация не записывается.
 */
static bool write_key_data(void)
{
	bool ret = false;
	int i;
	if (make_key_buf()){
		for (i = 0; (i < NR_WTRIES) && loop_flag; i++){
			_log("Writing data to secure module. Try #%d\n", i);
			if (usbkey_open(0)){
				if (!usbkey_write(0, key_buf))
					_log("Error writing secure module: %s.\n",
						strerror(errno));
				else if (!usbkey_read(0, key_rx_buf, 0, USB_KEY_BLOCKS))
					_log("Error reading secure module: %s.\n",
						strerror(errno));
				else if (memcmp(key_buf, key_rx_buf, sizeof(key_buf)) != 0)
					_log("Secure module data check fail.\n");
				else{
					_log("Secure module was successfully wrote and checked.\n");
					ret = true;
					break;
				}
				usbkey_close(0);
			}else
				_log("Fail to open secure module: %s.\n",
					strerror(errno));
		}
	}
	return ret;
}

/* Флаг необходимости обновления данных в модуле безопасности */
static bool need_update = true;

/* Обновление данных в модуле безопасности */
static void do_update(void)
{
	char *s;
	bool flag = false;
	_log("Start to upgrade key distribution.\n");
	need_update = false;
	s = scan_new_dst_dir();
	if ((s != NULL) && (rename(s, IPLIR_DST) == 0))
		flag = make_key_buf() && write_key_data();
	rename_upgrd_files(flag);
	system("rm -rf " NEW_DST_DIR_NAME " >/dev/null 2>/dev/null");
}

/* Обработчик сигнала SIGTERM */
static void sigterm_handler(int n)
{
	struct sigaction sa = {
		.sa_handler	= SIG_DFL
	};
	sigaction(SIGUPD, &sa, NULL);
	loop_flag = false;
}

/* Обработчик сигнала SIGUPD */
static void sigupd_handler(int n)
{
	need_update = true;
}

/* Установка обработчиков сигналов */
static void set_sig_handlers(void)
{
	static struct sigaction sa_term = {
		.sa_handler	= sigterm_handler,
		.sa_flags	= SA_RESETHAND
	};
	static struct sigaction sa_upd = {
		.sa_handler	= sigupd_handler,
		.sa_flags	= SA_RESTART
	};
	sigaction(SIGTERM, &sa_term, NULL);
	sigaction(SIGUPD, &sa_upd, NULL);
}

/* Если существует PID_FILE_NAME, то уже запущена копия программы */
static bool has_pid_file(char *name)
{
	struct stat st;
	return stat(name, &st) == 0;
}

int main()
{
	if (has_pid_file(PID_FILE_NAME)){
		fprintf(stderr, "Application is already launched.\n");
		return 1;
	}else if (!log_open())
		fprintf(stderr, "Fail to open _log; debug messages will be supressed.\n");
	_log("\nApplication started.\n");
	daemon(0, 0);
	set_sig_handlers();
	if (create_pid_file(PID_FILE_NAME)){
		while (loop_flag){
			if (need_update)
				do_update();
			else
				usleep(1000);
		}
		unlink(PID_FILE_NAME);
		_log("Application terminated.\n");
	}
	log_close();
	return 0;
}
