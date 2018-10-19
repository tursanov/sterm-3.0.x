/*
 * �ணࠬ�� 㤠����� ��業��� ��� � �ନ���� ��� �ᯮ�짮�����
 * ����� ������᭮��. �� 㤠����� � MBR �����뢠���� ᯥ樠��� 䫠�,
 * � ���짮��⥫� �뤠���� 32-���筮� ��⭠����筮� �᫮. ��᫮
 * �ନ����� �� �᭮�� ⥪�饩 ����, ⠡���� ��砩��� �ᥫ (rndtbl.c)
 * � ��� �����᪮�� ����� �ନ����. (c) gsr 2006.
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
#include "base64.h"
#include "colortty.h"
#include "license.h"
#include "md5.h"
#include "paths.h"
#include "rndtbl.h"
#include "tki.h"

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
	int dev = open(DOM_DEV, O_RDWR);
	if (dev == -1)
		dev = open(DOC_DEV, O_RDWR);
	if (dev == -1)
		fprintf(stderr, tRED "�訡�� ������ ���⥫� �ନ����\n"
			ANSI_DEFAULT);
	return dev;
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

/* �������� 䠩�� ������᪮� ��業��� */
static bool rm_lic_file(void)
{
	struct stat st;
	if (stat(BANK_LICENSE, &st) == -1){
		if (errno == ENOENT)
			return true;
		else{
			fprintf(stderr, tRED
				"�訡�� ����祭�� ���ଠ樨 � ��業��� ���: %s\n"
				ANSI_DEFAULT, strerror(errno));
			return false;
		}
	}else if (!S_ISREG(st.st_mode)){
		fprintf(stderr, tRED "������ �ଠ� ��業��� ���\n" ANSI_DEFAULT);
		return false;
	}else if (unlink(BANK_LICENSE) == -1){
		fprintf(stderr, tRED "�訡�� 㤠����� ��業��� ���: %s\n"
			ANSI_DEFAULT, strerror(errno));
		return false;
	}else
		return true;
}

/* �������� �� �����᪮�� ����� �ନ���� */
static bool read_term_number(struct md5_hash *number)
{
	uint8_t nr[TERM_NUMBER_LEN], buf[32];
	int l;
	if (!read_tki(STERM_TKI_NAME, false))
		return false;
	get_tki_field(&tki, TKI_NUMBER, nr);
	l = base64_encode(nr, sizeof(nr), buf);
	encrypt_data(buf, l);
	get_md5(buf, l, number);
	return true;
}

/* ��ନ஢���� �᫠ ��� ���⢥ত���� 㤠����� ��業��� */
static bool mk_del_hash(uint8_t *buf)
{
	struct md5_hash number;
	uint8_t *p = (uint8_t *)&number;
	uint32_t t;
	int i, index;
	if (!read_term_number(&number))
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

/* �뢮� ����� �� �࠭ */
static void print_number(uint8_t *number)
{
	int i;
	printf(CLR_SCR);
	printf(ANSI_PREFIX "12;H" tCYA
"               ������᪠� ��業��� 㤠���� � ��襣� �ନ����.\n"
"   ��� ���⢥ত���� 㤠����� ᮮ���, ��������, �����, 㪠����� ����,\n"
"                              � �� ��� \"������\"\n");
	printf(ANSI_PREFIX "16;20H" tYLW);
	for (i = 0; i < 16; i++){
		if ((i > 0) && (i < 15) && ((i % 2) == 0))
			putchar('-');
		printf("%.2hhX", number[i]);
	}
	printf(ANSI_DEFAULT "\n");
}

/* ���⪠ �室���� ��⮪� */
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

/* ���⪠ �室���� ��⮪� � �⥭�� �� ���� ᨬ���� */
static int flush_getchar(void)
{
	flush_stdin();
	return getchar();
}

/* ����� ���⢥ত���� 㤠����� ��業��� */
static bool ask_confirmation(void)
{
	int c;
	while (true){
		printf(CLR_SCR);
		printf(	ANSI_PREFIX "10;H" tYLW
"      �� ����⢨⥫쭮 ��� 㤠���� ��業��� ��� � ������� �ନ���� ?\n"
			ANSI_PREFIX "11;30H" tGRN "1" tYLW " -- ��;\n"
			ANSI_PREFIX "12;30H" tGRN "2" tYLW " -- ���\n"
			ANSI_PREFIX "13;30H" tWHT "��� �롮�: " ANSI_DEFAULT);
		c = flush_getchar();
		switch (c){
			case '1':
				return true;
			case '2':
				printf(CLR_SCR ANSI_PREFIX "12;23H");
				printf(tRED "��業��� ��� �� �㤥� 㤠����\n" ANSI_DEFAULT);
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
