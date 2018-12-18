#ifndef AD_H
#define AD_H

#include <stdint.h>
#include <stdio.h>
#include "sysdefs.h"
#include "list.h"

struct C;
struct L;
struct K;

// составляющая
typedef struct L {
    char *s;        // Название составляющей
    uint8_t p;      // Код признака расчёта
    uint8_t r;      // Код признака способа расчёта
    int64_t t;      // Стоимость составляющей, включая НДС
    uint8_t n;      // Стоимость составляющей, включая НДС
    int64_t c;      // Величина НДС за составляющую
} L;

// создание составляющей
extern L *L_create(void);
// удаление составляющей
extern void L_destroy(L *l);
// запись составляющей в файл
extern int L_save(FILE *f, L *l);
// загрузка составляющей из файла
extern L *L_load(FILE *f);

// документ
typedef struct K {
	struct list_t llist;    // список составляющих
    uint8_t o;          // Операция
    int64_t d;          // Номер оформляемого документа или КРС при возврате
    int64_t r;          // Номер документа, для которого оформляется дубликат, или возвращаемого документа или гасимого документа или гасимой КРС возврата
    int64_t i1;          // индекс
	int64_t i2;          // индекс
	int64_t i21;          // индекс
	int64_t p;          // ИНН перевозчика
    char *h;            // телефон перевозчика
    uint8_t m;          // способ оплаты
    char *t;            // номер телефона пассажира
    char *e;            // адрес электронной посты пассажира
} K;

// создание информации о документе
extern K *K_create(void);
// удаление информации о документе
extern void K_destroy(K *k);
// добавление составляющей в документ
extern bool K_addL(K *k, L* l);
// разделение документа на 2 по признаку расчета
extern K *K_divide(K *k, uint8_t p);
// проверка на равенство по всем L
extern bool K_equalByL(K *k1, K *k2);

// запись документа в файл
extern int K_save(FILE *f, K *k);
// загрузка документа из файла
extern K *K_load(FILE *f);

// сумма
typedef struct S {
    int64_t a; // общая сумма
    int64_t n; // сумма наличных
    int64_t e; // сумма электронных
    int64_t p; // сумма в зачет ранее внесенных средств
} S;


// чек
typedef struct C {
    list_t klist;       // список тэгов K
    uint64_t p;     // ИНН переводчика
    char *h;        // телефон перевозчика
    uint8_t t1054;  // признак расчета
    uint8_t t1055;  // применяемая система налогообложения
    S sum;          // сумма
    char *pe;       // Тел. или e-mail покупателя
} C;

extern C* C_create(void);
extern void C_destroy(C *c);
extern bool C_addK(C *c, K *k);

extern int C_save(FILE *f, C *c);
extern C* C_load(FILE *f);

// данные кассира
typedef struct P1 {
    char *i;
    char *p;
    char *t;
    char *c;
} P1;

extern P1 *P1_create(void);
extern void P1_destroy(P1 *p1);
extern int P1_save(FILE *f, P1 *p1);
extern P1* P1_load(FILE *f);

// массив 64-битных величин
typedef struct {
	int64_t *values;
	size_t capacity;
	size_t count;
} int64_array_t;
#define INIT_INT64_ARRAY { NULL, 0, 0 }

extern int int64_array_init(int64_array_t *array);
extern int int64_array_clear(int64_array_t *array);
extern int int64_array_free(int64_array_t *array);
// добавить значение (unique - добавить только то, значение, которое отсутствует в списке
extern int int64_array_add(int64_array_t *array, int64_t v, bool unique);

typedef struct string_array_t {
	char **values;
	size_t capacity;
	size_t count;
} string_array_t;
#define INIT_STRING_ARRAY { NULL, 0, 0 }

extern int string_array_init(string_array_t *array);
extern int string_array_clear(string_array_t *array);
extern int string_array_free(string_array_t *array);
// добавить значение (unique - добавить только то, значение, которое отсутствует в списке
// cnv - 0 - добавить как есть, 1 - преобразовать в нижний регистр, 
// 2 - преобразовать в верхний регистр)
extern int string_array_add(string_array_t *array, const char *s, bool unique, int cnv);

// Корзина фискального приложения
typedef struct AD {
    P1* p1;              // данные кассира
    uint8_t t1055;
    char * t1086;
#define MAX_SUM 4
    S sum[MAX_SUM];     // сумма
    list_t clist;       // список чеков
	int64_array_t docs;	// список документов
	string_array_t phones; // список телефонов
	string_array_t emails; // список e-mail
} AD;

// ссылка на текущую корзину
extern AD* _ad;

// удаление корзины
extern void AD_destroy(void);
// сохранение корзины на диск
extern int AD_save(void);
// загрузка корзины с диска
extern int AD_load(uint8_t t1055);
// установка значения для P1
extern void AD_setP1(P1 *p1);
#define AD_doc_count() (_ad ? _ad->docs.count : 0)

// удаление из корзины документа
extern int AD_delete_doc(int64_t doc);

extern void AD_calc_sum();

// callback для обработки XML
extern int kkt_xml_callback(uint32_t check, int evt, const char *name, const char *val);

#endif // AD_H
