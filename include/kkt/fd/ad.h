#ifndef AD_H
#define AD_H

#include <stdint.h>
#include <stdio.h>
#include "list.h"

// ��⠢�����
typedef struct L {
    char *s;        // �������� ��⠢���饩
    uint8_t p;      // ��� �ਧ���� �����
    uint8_t r;      // ��� �ਧ���� ᯮᮡ� �����
    int64_t t;      // �⮨����� ��⠢���饩, ������ ���
    uint8_t n;      // �⮨����� ��⠢���饩, ������ ���
    int64_t c;      // ����稭� ��� �� ��⠢������
} L;

// ᮧ����� ��⠢���饩
extern L *L_create(void);
// 㤠����� ��⠢���饩
extern void L_destroy(L *l);
// ������ ��⠢���饩 � 䠩�
extern int L_save(FILE *f, L *l);
// ����㧪� ��⠢���饩 �� 䠩��
extern L *L_load(FILE *f);

// ���㬥��
typedef struct K {
    struct list_t llist;    // ᯨ᮪ ��⠢�����
    uint8_t o;          // ������
    int64_t d;          // ����� ��ଫ塞��� ���㬥�� ��� ��� �� ������
    int64_t r;          // ����� ���㬥��, ��� ���ண� ��ଫ���� �㡫����, ��� �����頥���� ���㬥�� ��� ��ᨬ��� ���㬥�� ��� ��ᨬ�� ��� ������
    int64_t i;          // ������
    int64_t p;          // ��� ��ॢ��稪�
    char *h;            // ⥫�䮭 ��ॢ��稪�
    uint8_t m;          // ᯮᮡ ������
    char *t;            // ����� ⥫�䮭� ���ᠦ��
    char *e;            // ���� ���஭��� ����� ���ᠦ��
} K;

// ᮧ����� ���ଠ樨 � ���㬥��
extern K *K_create(void);
// 㤠����� ���ଠ樨 � ���㬥��
extern void K_destroy(K *k);
// ���������� ��⠢���饩 � ���㬥��
extern bool K_addL(K *k, L* l);
// ࠧ������� ���㬥�� �� 2 �� �ਧ���� ����
extern K *K_divide(K *k, uint8_t p);
// �஢�ઠ �� ࠢ���⢮ �� �ᥬ L
extern bool K_equalByL(K *k1, K *k2);

// ������ ���㬥�� � 䠩�
extern int K_save(FILE *f, K *k);
// ����㧪� ���㬥�� �� 䠩��
extern K *K_load(FILE *f);

// �㬬�
typedef struct S {
    int64_t a; // ���� �㬬�
    int64_t n; // �㬬� �������
    int64_t e; // �㬬� ���஭���
    int64_t p; // �㬬� � ���� ࠭�� ���ᥭ��� �।��
} S;

// �ਡ����� � dst src
extern void S_add(S *dst, S*src);
// ������ �� dst src
extern void S_subtract(S* dst, S*src);
// �ਡ����� ���祭�� � �㬬� (� ����ᨬ��� �� ���� ����)
extern void S_addValue(uint8_t m, int64_t value, size_t count, ...);
// ������ �� �㬬� ���祭�� (� ����ᨬ��� �� ���� ����)
extern void S_subtractValue(uint8_t m, int64_t value, size_t count, ...);

// 祪
typedef struct C {
    list_t klist;       // ᯨ᮪ ��� K
    uint64_t p;     // ��� ��ॢ��稪�
    char *h;        // ⥫�䮭 ��ॢ��稪�
    uint8_t t1054;  // �ਧ��� ����
    uint8_t t1055;  // �ਬ��塞�� ��⥬� ���������������
    S sum;          // �㬬�
    char * t1086;  // ���祭�� �������⥫쭮�� ४����� ���짮��⥫�
    char *pe;       // ���. ��� e-mail ���㯠⥫�
} C;

extern C* C_create(void);
extern void C_destroy(C *c);
extern bool C_addK(C *c, K *k);

extern int C_save(FILE *f, C *c);
extern C* C_load(FILE *f);

// ����� �����
typedef struct P1 {
    char *i;
    char *p;
    char *t;
    char *s;
} P1;

extern P1 *P1_create(void);
extern void P1_destroy(P1 *p1);
extern int P1_save(FILE *f, P1 *p1);
extern P1* P1_load(FILE *f);

// ��২�� �᪠�쭮�� �ਫ������
typedef struct AD {
    P1* p1;              // ����� �����
    uint8_t t1055;
    char * t1086;
#define MAX_SUM 4
    S sum[MAX_SUM];     // �㬬�
    list_t clist;           // ᯨ᮪ 祪��
} AD;

// 㤠����� ��২��
extern void AD_destroy(void);
// ��࠭���� ��২�� �� ���
extern int AD_save(void);
// ����㧪� ��২�� � ��᪠
extern int AD_load(void);
// ��⠭���� ���祭�� ��� �� T1086
extern void AD_setT1086(const char *i, const char *p, const char *t);

// callback ��� ��ࠡ�⪨ XML
extern int kkt_xml_callback(uint32_t check, int evt, const char *name, const char *val);

#endif // AD_H
