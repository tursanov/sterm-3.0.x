#ifndef FD_H
#define FD_H

#include "kkt/fd/ad.h"

// ०��� ࠡ���
#define REG_MODE_CRYPTO		0x01
#define REG_MODE_OFFLINE	0x02
#define REG_MODE_AUTOMAT	0x04
#define REG_MODE_SERVICE	0x08
#define REG_MODE_BSO		0x10
#define REG_MODE_INTERNET	0x20

// ��ࠬ���� ॣ����樨
typedef struct fd_registraton_params_t {
    char user_name[256+1]; // ������������ ���짮��⥫�
    char user_inn[12+1]; // ��� ���짮��⥫�
    char pay_address[256+1]; // ���� ���⮢
    char pay_place[256+1]; // ���� ���⮢
    uint8_t tax_systems; // ��⥬� ���������������
    char reg_number[20+1]; // ���
    uint8_t reg_modes; // ०��� ࠡ���
    char automat_number[20+1]; // ����� ��⮬��
    char cashier[64+1]; // �����
    char cashier_inn[12+1]; // ��� �����
    char tax_service_site[256+1]; // ���� ᠩ� ���
    char cheque_sender_email[64+1]; // ���� �. ����� ��ࠢ�⥫� 祪��
    char ofd_name[256+1]; // ������������ ���
    char ofd_inn[12+1]; // ��� ���
    uint8_t rereg_reason; // ��稭� ���ॣ����樨 (��� 0, �᫨ �믮������ ॣ������)
} fd_registration_params_t;

// ��ࠬ���� ������/������� ᬥ��
typedef struct fd_shift_params_t {
    char cashier[64+1]; // �����
    char post[64+1]; // ���������
    char cashier_inn[12+1]; // ��� �����
} fd_shift_params_t;

// ��ࠬ���� ������� ��
typedef struct fd_close_fs_params_t {
    char cashier[64+1]; // �����
    char cashier_inn[12+1]; // ��� �����
} fd_close_fs_params_t;

// ⨯ �㬬�
typedef enum fd_sum_type_t {
    fd_sum_type_cash, // ������
    fd_sum_type_electronic, // ���஭��
    fd_sum_type_prepayment, // �।�����
    fd_sum_type_postpayment, // ���⮯���
    fd_sum_type_b2b, // ����筮� ���⠢�����
    fd_sum_type_max
} fd_sum_type_t;

// ⨯ ���
typedef enum fd_vat_rate_t {
    fd_vat_rate_18 = 1, // ��� 18%
    fd_vat_rate_10, // ��� 10%
    fd_vat_rate_18_118, // ��� 18/118
    fd_vat_rate_10_110, // ��� 10/110
    fd_vat_rate_0, // ��� 0%
    fd_vat_rate_no, // ��� �� ����������
    fd_vat_rate_max = fd_vat_rate_no
} fd_vat_rate_t;

// ��ࠬ���� 祪� ���४樨
typedef struct fd_cheque_corr_params_t {
    uint8_t pay_type; // �ਧ��� ����
    uint8_t tax_system; // ��⥬� ���������������
    uint8_t corr_type; // ⨯ ���४樨
    struct {
        char descr[256+1]; // ���ᠭ�� ���४樨
        time_t date; // ���
        char reg_number[32+1]; // ����� �।��ᠭ��
    } corr_reason; // �᭮����� ��� ���४樨
    uint64_t sum[fd_sum_type_max]; // �㬬�
    int64_t nds[fd_vat_rate_max]; // �⠢�� ���. �᫨ �⠢�� �� 㪠����, � ���祭�� �.�. ����⥫��
} fd_cheque_corr_params_t;

// ����祭�� ��᫥���� �訡��
int fd_get_last_error(const char **error);
// ॣ������/���ॣ������
int fd_registration(fd_registration_params_t *params);
// ����⨥ ᬥ��
int fd_open_shift(fd_shift_params_t *params);
// �����⨥ ᬥ��
int fd_close_shift(fd_shift_params_t *params);
// ���� � ⥪��� �����
int fd_calc_report();


#endif
