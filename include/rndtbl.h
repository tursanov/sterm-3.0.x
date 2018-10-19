/*
 * ������ ��砩��� �ᥫ ��� �ணࠬ�� ���� ��業��� ���.
 * ������ ������ � ⠡��� ��⮨� �� 3-� 32-ࠧ�來�� ��砩��� �ᥫ.
 * (c) gsr 2006
 */

#if !defined RNDTBL_H
#define RNDTBL_H

#include "sysdefs.h"

/* ������ � ⠡��� ��砩��� �ᥫ */
struct rnd_rec {
	uint32_t a;
	uint32_t b;
	uint32_t c;
};

/* ������⢮ ����ᥩ � ⠡��� ��砩��� �ᥫ */
#define NR_RND_REC		100

extern struct rnd_rec rnd_tbl[NR_RND_REC];

#endif		/* RNDTBL_H */
