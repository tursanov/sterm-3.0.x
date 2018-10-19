/* ��⠭���� �� �ନ��� ������᪮� ��業���. (c) gsr 2005 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "license.h"

/* �����稢��� ���ᨬ��쭮� ������⢮ ��業��� */
#if defined MAX_LICENSES
#undef MAX_LICENSES
#endif

#define MAX_LICENSES		10000

/* ���� ��� ࠡ��� � MBR */
static uint8_t mbr[SECTOR_SIZE];

/* �⥭�� MBR ��������� ���ன�⢠ */
static bool read_mbr(int dev)
{
	return	(lseek(dev, 0, SEEK_SET) == 0) &&
		(read(dev, mbr, sizeof(mbr)) == sizeof(mbr));
}

/* ������ MBR ��������� ���ன�⢠ */
static bool write_mbr(int dev)
{
	return	(lseek(dev, 0, SEEK_SET) == 0) &&
		(write(dev, mbr, sizeof(mbr)) == sizeof(mbr));
}

/* ����⨥ ���ன�⢠ ��� �⥭��/����� LIC_SIGNATURE */
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

/* �஢�ઠ ������⢨� �ਧ���� ��⠭���� ��業��� */
static bool check_lic_signature(int dev)
{
	if (read_mbr(dev))
		return *(uint16_t *)(mbr + LIC_SIGNATURE_OFFSET) == LIC_SIGNATURE;
	else
		return true;	/* �㤥� �����, �� ��業��� 㦥 ��⠭������ */
}

/* ������ �ਧ���� ��⠭�������� ��業��� */
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

/* �⥭�� 䠩�� ��業��� */
static int read_license_file(char *name, struct license_info *licenses)
{
	struct stat st;
	div_t d;
	int fd, ret = -1;
	if (stat(name, &st) == -1){
		fprintf(stderr, "�訡�� ����祭�� ���ଠ樨 � 䠩�� %s: %s.\n",
			name, strerror(errno));
		return ret;
	}else if (!S_ISREG(st.st_mode)){
		fprintf(stderr, "%s �� ���� ����� 䠩���.\n", name);
		return ret;
	}
	d = div(st.st_size, sizeof(struct license_info));
	if ((d.quot == 0) || (d.rem != 0)){
		fprintf(stderr, "���� %s ����� ������ ࠧ���.\n", name);
		return ret;
	}else if (d.quot > MAX_LICENSES){
		fprintf(stderr, "��᫮ ��業��� �� ����� �ॢ���� %d.\n",
			MAX_LICENSES);
		return ret;
	}
	fd = open(name, O_RDONLY);
	if (fd == -1){
		fprintf(stderr, "�訡�� ������ %s ��� �⥭��: %s.\n",
			name, strerror(errno));
		return ret;
	}
	if (read(fd, licenses, st.st_size) == st.st_size)
		ret = d.quot;
	if (ret == -1)
		fprintf(stderr, "�訡�� �⥭�� �� %s: %s.\n",
			name, strerror(errno));
	close(fd);
	return ret;
}

/* �⥭�� �� �����᪮�� ����� �ନ���� */
static bool read_term_number(char *name, struct md5_hash *number)
{
	struct stat st;
	int fd;
	bool ret;
	if (stat(name, &st) == -1){
		fprintf(stderr, "�訡�� ����祭�� ���ଠ樨 � 䠩�� %s: %s.\n",
			name, strerror(errno));
		return false;
	}else if (!S_ISREG(st.st_mode)){
		fprintf(stderr, "%s �� ���� ����� 䠩���.\n", name);
		return false;
	}
	if (st.st_size != sizeof(struct md5_hash)){
		fprintf(stderr, "���� %s ����� ������ ࠧ���.\n", name);
		return false;
	}
	fd = open(name, O_RDONLY);
	if (fd == -1){
		fprintf(stderr, "�訡�� ������ %s ��� �⥭��: %s.\n",
			name, strerror(errno));
		return false;
	}
	ret = read(fd, number, sizeof(*number)) == sizeof(*number);
	if (!ret)
		fprintf(stderr, "�訡�� �⥭�� �� %s: %s.\n",
			name, strerror(errno));
	close(fd);
	return ret;
}

/*
 * ���� ������᪮� ��業��� �� ��� �����᪮�� �����. �᫨ �� �����᪮��
 * ����� ��������� � ᯨ᪥ ����� ������ ࠧ�, ����� ��業��� ��� �⮣�
 * �ନ���� �뫠 㤠���� � �� ������ ������� � MBR �ਧ��� 㤠�����
 * ��業���.
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

/* �뢮� ������᪮� ��業��� � stdout */
static void write_license(struct md5_hash *license)
{
	write(STDOUT_FILENO, license, sizeof(*license));
}

int main(int argc, char **argv)
{
	int ret = 1, dev;
	struct md5_hash number, *license;
	if (argc != 2)
		fprintf(stderr, "������ ��� 䠩�� ᯨ᪠ ��業��� ��� ࠡ��� � ���.\n");
	else if (!read_term_number(TERM_NUMBER_FILE, &number))
		fprintf(stderr, "�� ������ ����� ������᭮��.\n");
	else{
		dev = open_term_dev();
		if (dev != -1){
			if (check_lic_signature(dev))
				fprintf(stderr, "��⠭���� ��業��� ��� ࠡ��� "
					"� ��� �� ����� �ନ��� ����������;\n"
					"�������, ��������, � �� ��� \"������\".\n");
			else{
				nr_licenses = read_license_file(argv[1], licenses);
				if (nr_licenses > 0){
					license = find_license(dev, licenses,
						nr_licenses, &number);
					if (license != NULL){
						write_license(license);
						ret = 0;
					}else
						fprintf(stderr, "�� ������� "
							"��業��� ��� �⮣� �ନ����.\n");
				}
			}
			close(dev);
		}
	}
	return ret;
}
