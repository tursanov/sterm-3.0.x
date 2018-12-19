#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "sysdefs.h"
#ifdef WIN32
#include "ad.h"
#else
#include "kkt/fd/ad.h"
#endif

#ifdef WIN32
#define strcasecmp _stricmp
#define strdup _strdup
#endif

static int strcmp2(const char *s1, const char *s2) {
    if (s1 == NULL && s2 == NULL)
        return 0;
    else if (s1 == NULL && s2 != NULL)
        return -1;
    else if (s1 != NULL && s2 == NULL)
        return 1;
    else 
        return strcmp(s1, s2);
}


static int save_string(FILE *f, char *s) {
    size_t len = s != NULL ? strlen(s) : 0;
    if (fwrite(&len, sizeof(len), 1, f) != 1)
        return -1;
    if (len > 0 && fwrite(s, len, 1, f) != 1)
        return -1;
    return 0;
}

static int load_string(FILE *f, char **ret) {
    size_t len;
    if (fread(&len, sizeof(len), 1, f) != 1)
        return -1;
	if (len > 0) {
		char *s = (char *)malloc(len + 1);
		if (s == NULL)
			return -1;

		if (fread(s, len, 1, f) != 1) {
			free(s);
			return -1;
		}
		s[len] = 0;

		*ret = s;
	} else
		*ret = NULL;
    
    return 0;
}

static int save_int(FILE *f, uint64_t v, size_t size) {
    if (fwrite(&v, size, 1, f) != 1)
        return -1;
    return 0;
}
#define SAVE_INT(f, v) save_int((f), (v), sizeof(v))

static int load_int(FILE *f, uint64_t *v, size_t size) {
    if (fread(v, size, 1, f) != 1)
        return -1;
    return 0;
}
#define LOAD_INT(f, v) load_int((f), (uint64_t *)&(v), sizeof(v))

static int save_list(FILE *f, list_t *list, list_item_func_t save_item_func) {
    if (SAVE_INT(f, list->count) < 0 ||
        list_foreach(list, f, save_item_func) < 0)
        return -1;
    return 0;
}

typedef void * (*load_item_func_t)(FILE *f);

static int load_list(FILE *f, list_t *list, load_item_func_t load_item_func) {
    size_t count = 0;
    if (LOAD_INT(f, count) < 0)
        return -1;
    
    for (size_t i = 0; i < count; i++) {
        void *obj = load_item_func(f);
        if (obj == NULL)
            return -1;
        if (list_add(list, obj) != 0)
            return -1;
    }
    return 0;
}

L *L_create(void) {
    L *l = (L *)malloc(sizeof(L));
    memset(l, 0, sizeof(L));

    return l;
}

void L_destroy(L *l) {
    if (l->s)
        free(l->s);
    free(l);
}

int L_save(FILE *f, L *l) {
    if (save_string(f, l->s) < 0 ||
        SAVE_INT(f, l->p) < 0 ||
        SAVE_INT(f, l->r) < 0 ||
        SAVE_INT(f, l->t) < 0 ||
        SAVE_INT(f, l->n) < 0 ||
        SAVE_INT(f, l->c) < 0)
        return -1;
    return 0;
}

L *L_load(FILE *f) {
    L *l = L_create();
    if (load_string(f, &l->s) < 0 ||
        LOAD_INT(f, l->p) < 0 ||
        LOAD_INT(f, l->r) < 0 ||
        LOAD_INT(f, l->t) < 0 ||
        LOAD_INT(f, l->n) < 0 ||
        LOAD_INT(f, l->c) < 0) {
        L_destroy(l);
        return NULL;
    }
    return l;
}

K *K_create(void) {
    K *k = (K *)malloc(sizeof(K));
    memset(k, 0, sizeof(K));
    k->llist.delete_func = (list_item_delete_func_t)L_destroy;
    return k;
}

void K_destroy(K *k) {
    list_clear(&k->llist);
    if (k->h)
        free(k->h);
    if (k->t)
        free(k->t);
    if (k->e)
        free(k->e);
    free(k);
}

bool K_addL(K *k, L* l) {
	if (list_add(&k->llist, l) == 0) {
		return true;
	}
	return false;
}

#define COPYSTR(s) ((s) != NULL ? strdup(s) : NULL)

K *K_divide(K *k, uint8_t p) {
	K *k1 = NULL;
	int count = 0;
	for (list_item_t *item = k->llist.head, *prev = NULL; item != NULL;) {
		L *l = LIST_ITEM(item, L);
		list_item_t *tmp = item;
		item = item->next;
		if (l->p == p) {
			if (prev != NULL)
				prev->next = item;
			if (item == NULL)
				k->llist.tail = prev;
			if (tmp == k->llist.head)
				k->llist.head = item;
			count++;
			
			if (k1 == NULL) {
				k1 = (K*)malloc(sizeof(K));
				memset(&k1->llist, 0, sizeof(k1->llist));
				k->llist.delete_func = (list_item_delete_func_t)L_destroy;

				k1->o = k->o;          // Операция
				k1->d = k->d;          // Номер оформляемого документа или КРС при возврате
				k1->r = k->r;          // Номер документа, для которого оформляется дубликат, или возвращаемого документа или гасимого документа или гасимой КРС возврата
				k1->i1 = k->i1;          // индекс
				k1->i2 = k->i2;          // индекс
				k1->i21 = k->i21;          // индекс
				k1->p = k->p;          // ИНН перевозчика

				k1->h = COPYSTR(k->h); // телефон перевозчика
				k1->m = k->m;          // способ оплаты
				k1->t = COPYSTR(k->t);            // номер телефона пассажира
				k1->e = COPYSTR(k->e);            // адрес электронной посты пассажира
			}

			list_add_item(&k1->llist, tmp);
		} else
			prev = tmp;
	}
	k->llist.count -= count;

    return k1;
}

static int L_compare(void *arg, L *l1, L *l2) {
	if ((strcmp2(l1->s, l2->s) == 0 &&
		l1->r == l2->r &&
		l1->t == l2->t &&
		l1->n == l2->n &&
		l1->c == l2->c))
		return 0;
	return 1;
}

bool K_equalByL(K *k1, K* k2) {
    return list_compare(&k1->llist, &k2->llist, NULL, (list_item_compare_func_t)L_compare) == 0;
}

int K_save(FILE *f, K *k) {
    if (save_list(f, &k->llist, (list_item_func_t)L_save) < 0 ||
        SAVE_INT(f, k->o) < 0 ||
        SAVE_INT(f, k->d) < 0 ||
        SAVE_INT(f, k->r) < 0 ||
        SAVE_INT(f, k->i1) < 0 ||
		SAVE_INT(f, k->i2) < 0 ||
		SAVE_INT(f, k->i21) < 0 ||
		SAVE_INT(f, k->p) < 0 ||
        save_string(f, k->h) < 0 ||
        SAVE_INT(f, k->m) < 0 ||
        save_string(f, k->t) < 0 ||
        save_string(f, k->e) < 0)
        return -1;
    return 0;
}

static void K_after_add(K *k) {
    int64_array_add(&_ad->docs, k->d, true);
    if (k->t != NULL) {
	    string_array_add(&_ad->phones, k->t, true, 0);
	}
    if (k->e != NULL) {
	    string_array_add(&_ad->emails, k->e, true, 1);
	}
}

K *K_load(FILE *f) {
    K *k = K_create();
    if (load_list(f, &k->llist, (load_item_func_t)L_load) < 0 ||
            LOAD_INT(f, k->o) < 0 ||
            LOAD_INT(f, k->d) < 0 ||
            LOAD_INT(f, k->r) < 0 ||
            LOAD_INT(f, k->i1) < 0 ||
			LOAD_INT(f, k->i2) < 0 ||
			LOAD_INT(f, k->i21) < 0 ||
			LOAD_INT(f, k->p) < 0 ||
            load_string(f, &k->h) < 0 ||
            LOAD_INT(f, k->m) < 0 ||
            load_string(f, &k->t) < 0 ||
            load_string(f, &k->e) < 0) {
        K_destroy(k);
        return NULL;
    }
    K_after_add(k);
    return k;
}


C* C_create(void) {
    C *c = (C *)malloc(sizeof(C));
    memset(c, 0, sizeof(C));
    c->klist.delete_func = (list_item_delete_func_t)K_destroy;
    
    return c;
}

void C_destroy(C *c) {
    list_clear(&c->klist);
    if (c->h != NULL)
        free(c->h);
    if (c->pe != NULL)
        free(c->pe);

	int64_array_clear(&_ad->docs);
	string_array_clear(&_ad->phones);
	string_array_clear(&_ad->emails);

    for (list_item_t *i = _ad->clist.head; i != NULL; i = i->next) {
		C *c1 = LIST_ITEM(i, C);
		if (c1 != c) {
		    for (list_item_t *j = c1->klist.head; j != NULL; j = j->next) {
		    	K *k = LIST_ITEM(j, K);
		    	K_after_add(k);
		    }
		}
    }
}

bool C_addK(C *c, K *k) {
	if (list_add(&c->klist, k) == 0) {
		return true;
	}
	return false;
}

int C_save(FILE *f, C *c) {
    if (save_list(f, &c->klist, (list_item_func_t)K_save) < 0 ||
            SAVE_INT(f, c->p) < 0 ||
            save_string(f, c->h) < 0 ||
            SAVE_INT(f, c->t1054) < 0 ||
            SAVE_INT(f, c->t1055) < 0 ||
            save_string(f, c->pe) < 0)
        return -1;
    return 0;
}

C * C_load(FILE *f) {
    C *c = C_create();
    if (load_list(f, &c->klist, (load_item_func_t)K_load) < 0 ||
            LOAD_INT(f, c->p) < 0 ||
            load_string(f, &c->h) < 0 ||
            LOAD_INT(f, c->t1054) < 0 ||
            LOAD_INT(f, c->t1055) < 0 ||
            load_string(f, &c->pe) < 0) {
        C_destroy(c);
        return NULL;
    }
    return c;
}

P1 *P1_create(void) {
    P1* p1 = (P1 *)malloc(sizeof(P1));
    memset(p1, 0, sizeof(P1));
    
    return p1;
}

void P1_destroy(P1 *p1) {
    if (p1->i != NULL)
        free(p1->i);
    if (p1->p != NULL)
        free(p1->p);
    if (p1->t != NULL)
        free(p1->t);
    if (p1->c != NULL)
        free(p1->c);
    free(p1);
}

int P1_save(FILE *f, P1 *p1) {
    if (save_string(f, p1->i) < 0 ||
        save_string(f, p1->p) < 0 ||
        save_string(f, p1->t) < 0 ||
        save_string(f, p1->c) < 0)
        return -1;
    return 0;
}

P1* P1_load(FILE *f) {
    P1 *p1 = P1_create();
    if (load_string(f, &p1->i) < 0 ||
        load_string(f, &p1->p) < 0 ||
        load_string(f, &p1->t) < 0 ||
        load_string(f, &p1->c) < 0)
        return NULL;
    printf("P1->i = %s\n, P1->p = %s, P1->t = %s, P1->c = %s\n",
            p1->i, p1->p, p1->t, p1->c);
    return p1;
}

int int64_array_init(int64_array_t *array) {
#define DEFAULT_CAPACITY	32
	array->capacity = DEFAULT_CAPACITY;
	array->values = (int64_t *)malloc(array->capacity * sizeof(int64_t));
	array->count = 0;

	return array->values != NULL ? 0 : -1;
}

int int64_array_clear(int64_array_t *array) {
	array->count = 0;
	return 0;
}

int int64_array_free(int64_array_t *array) {
	if (array->values)
		free(array->values);
	array->values = NULL;
	array->capacity = array->count = 0;
	return 0;
}

int int64_array_add(int64_array_t *array, int64_t v, bool unique) {
	if (unique) {
		int64_t *p = array->values;
		for (size_t i = 0; i < array->count; i++, p++)
			if (*p == v)
				return 0;
	}
	if (array->count == array->capacity) {
		array->capacity *= 2;
		array->values = (int64_t *)realloc(array->values, array->capacity);
		if (array->values == NULL)
			return -1;
	}
	array->values[array->count] = v;
	array->count++;
	return 1;
}

int string_array_init(string_array_t *array) {
#define DEFAULT_CAPACITY	32
	array->capacity = DEFAULT_CAPACITY;
	array->values = (char **)malloc(array->capacity * sizeof(char *));
	array->count = 0;

	return array->values != NULL ? 0 : -1;
}

int string_array_clear(string_array_t *array) {
	for (size_t i = 0; i < array->count; i++)
		if (array->values[i])
			free(array->values[i]);
	memset(array->values, 0, sizeof(char *) * array->count);
	array->count = 0;
	return 0;
}

int string_array_free(string_array_t *array) {
	for (size_t i = 0; i < array->count; i++)
		if (array->values[i])
			free(array->values[i]);

	if (array->values)
		free(array->values);
	array->values = NULL;
	array->capacity = array->count = 0;
	return 0;
}

int string_array_add(string_array_t *array, const char *v, bool unique, int cnv) {
	if (unique) {
		char **p = array->values;
		for (size_t i = 0; i < array->count; i++, p++) {
			if (strcasecmp(*p, v) == 0)
				return 0;
		}
	}
	if (array->count == array->capacity) {
		array->capacity *= 2;
		array->values = (char **)realloc(array->values, array->capacity);
		if (array->values == NULL)
			return -1;
	}
	char *str = strdup(v);

	if (cnv != 0) {
		for (char *s = str; *s; s++) {
			if (cnv == 1)
				s[0] = tolower(s[0]);
			else
				s[0] = toupper(s[0]);
		}
	}
	array->values[array->count] = str;

	array->count++;
	return 1;
}

AD* _ad = NULL;

int AD_create(uint8_t t1055) {
    _ad = (AD *)malloc(sizeof(AD));
    if (_ad == NULL)
        return -1;
    memset(_ad, 0, sizeof(AD));
    
	if (int64_array_init(&_ad->docs) != 0)
		return -1;
	if (string_array_init(&_ad->phones) != 0)
		return -1;
	if (string_array_init(&_ad->emails) != 0)
		return -1;

	_ad->t1055 = t1055;
    _ad->clist.delete_func = (list_item_delete_func_t)C_destroy;

    return 0;
}

void AD_destroy() {
    list_clear(&_ad->clist);
    if (_ad->p1 != NULL)
        P1_destroy(_ad->p1);
    if (_ad->t1086 != NULL)
        free(_ad->t1086);
    int64_array_free(&_ad->docs);
    string_array_free(&_ad->phones);
    string_array_free(&_ad->emails);
    free(_ad);
    _ad = NULL;
}

#ifdef WIN32
#define FILE_NAME   "ad.bin"
#else
#define FILE_NAME   "/home/sterm/ad.bin"
#endif

int AD_save() {
    int ret = -1;
    FILE *f = fopen(FILE_NAME, "wb");
    
    if (f == NULL) {
        return -1;
    }
    
    if (SAVE_INT(f, (uint8_t)(_ad->p1 != 0 ? 1 : 0)) < 0 ||
        (_ad->p1 != NULL && P1_save(f, _ad->p1) < 0) ||
        save_string(f, _ad->t1086) < 0 ||
        save_list(f, &_ad->clist, (list_item_func_t)C_save) < 0)
        ret = -1;
    else
        ret = 0;

    fclose(f);
    return ret;
}

int AD_load(uint8_t t1055, bool clear) {
    if (_ad != NULL)
        AD_destroy();
    if (AD_create(t1055) < 0)
        return -1;

	if (clear)
		return 0;

    FILE *f = fopen(FILE_NAME, "rb");
    if (f == NULL)
        return -1;
    int ret;
    uint8_t hasP1 = 0;
    
    if (LOAD_INT(f, hasP1) < 0 ||
        (hasP1 && (_ad->p1 = P1_load(f)) == NULL) ||
        load_string(f, &_ad->t1086) < 0 ||
        load_list(f, &_ad->clist, (load_item_func_t)C_load) < 0)
        ret = -1;
    else
        ret = 0;

	AD_calc_sum();
        
    printf("AD_load: %d, ad.C.count: %d, ad.docs.count: %d\n", ret, _ad->clist.count,
    	_ad->docs.count);
    
    fclose(f);
    return ret;
}

void AD_setP1(P1 *p1) {
	if (_ad->p1 != NULL)
		P1_destroy(_ad->p1);
	_ad->p1 = p1;

    if (_ad->p1 != NULL) {
        size_t l = 0;
        if (_ad->p1->i)
            l += strlen(_ad->p1->i);
        if (_ad->p1->p)
            l += strlen(_ad->p1->p);
        if (_ad->p1->t)
            l += strlen(_ad->p1->t);

        printf("l = %d\n", l);

		if (_ad->t1086)
			free(_ad->t1086);
        _ad->t1086 = (char *)malloc(l + 1);
        sprintf(_ad->t1086, "%s%s%s",
                _ad->p1->i ? _ad->p1->i : "",
                _ad->p1->p ? _ad->p1->p : "",
                _ad->p1->t ? _ad->p1->t : "");
    }

	AD_save();
}

int AD_delete_doc(int64_t doc) {
	int count = 0;
    for (list_it_t i1 = LIST_IT(&_ad->clist); !LIST_IT_END(i1); list_it_next(&i1)) {
        C *c = LIST_IT_OBJ(i1, C);
		for (list_it_t i2 = LIST_IT(&c->klist); !LIST_IT_END(i2); list_it_next(&i2)) {
			K *k = LIST_IT_OBJ(i2, K);
			if (k->d == doc) {
				list_it_remove(&i2);
				count++;
            }
        }
		if (c->klist.count == 0)
			list_it_remove(&i1);
	}

	_ad->docs.count = 0;

	for (list_item_t *li1 = _ad->clist.head; li1 != NULL; li1 = li1->next) {
		C *c = LIST_ITEM(li1, C);
		for (list_item_t *li2 = c->klist.head; li2 != NULL; li2 = li2->next) {
			K *k = LIST_ITEM(li2, K);
			int64_array_add(&_ad->docs, k->d, true);
		}
	}

	AD_calc_sum();
	
    if (count)
    	AD_save();
   	return count;
}

C* AD_getCheque(K *k, uint8_t t1054, uint8_t t1055) {
    for (list_item_t *i = _ad->clist.head; i != NULL; i = i->next) {
        C *c = (C *)i->obj;
        if (c->p == k->p && strcmp2(c->h, k->h) == 0 && c->t1054 == t1054 && c->t1055 == t1055)
            return c;
    }
    return NULL;
}

int AD_makeCheque(K *k, int64_t d, uint8_t t1054, uint8_t t1055) {
    if (k->llist.count == 0)
        return 0;
    
    k->d = d;
    C* c = AD_getCheque(k, t1054, t1055);
    
    if (c == NULL) {
    	printf("create new cheque\n");

        c = C_create();
        c->p = k->p;
        c->h = strdup(k->h);
        c->t1054 = t1054;
        c->t1055 = t1055;

        list_add(&_ad->clist, c);
    } else
    	printf("cheque found\n");

    list_add(&c->klist, k);
    K_after_add(k);

    printf("AD CH: %d\n", _ad->clist.count);

    return 0;
}

int AD_makeAnnul(K *k, uint8_t o, uint8_t t1054, uint8_t t1055) {
    for (list_it_t i1 = LIST_IT(&_ad->clist); !LIST_IT_END(i1); list_it_next(&i1)) {
        C *c = LIST_IT_OBJ(i1, C);
        if (c->p == k->p && strcmp2(c->h, k->h) == 0 && c->t1054 == t1054 && c->t1055 == t1055) {
            for (list_it_t i2 = LIST_IT(&c->klist); !LIST_IT_END(i2); list_it_next(&i2)) {
                K *k1 = LIST_IT_OBJ(i2, K);
                if (k1->i1 == k->r && k1->m == k->m && k1->o == o) {
                    if (K_equalByL(k, k1)) {
                        list_it_remove(&i2);
                        if (c->klist.count == 0)
                            list_it_remove(&i1);
						K_destroy(k);
						return 0;
					}
                }
            }
        }
    }
    AD_makeCheque(k, k->r, 2, t1055);
    return 0;
}

int AD_makeAnnulReturn(K *k, uint8_t o, uint8_t t1054, uint8_t t1055) {
	for (list_it_t i1 = LIST_IT(&_ad->clist); !LIST_IT_END(i1); list_it_next(&i1)) {
		C *c = LIST_IT_OBJ(i1, C);
		if (c->p == k->p && strcmp2(c->h, k->h) == 0 && c->t1054 == t1054 && c->t1055 == t1055) {
			for (list_it_t i2 = LIST_IT(&c->klist); !LIST_IT_END(i2); list_it_next(&i2)) {
				K *k1 = LIST_IT_OBJ(i2, K);
				if ((k1->i2 == k->r || k1->i21 == k->r) && k1->m == k->m && k1->o == o) {
					if (K_equalByL(k, k1)) {
						list_it_remove(&i2);
						if (c->klist.count == 0)
							list_it_remove(&i1);
						K_destroy(k);
						return 0;
					}
				}
			}
		}
	}
	AD_makeCheque(k, k->r, t1054 == 2 ? 1 : 2, t1055);
	return 0;
}

int AD_processO(K *k) {
    K *k1, *k2;
	int64_t d, r;
    
    printf("process new K. K->o = %d\n", k->o);
    
    switch (k->o) {
        case 1:
            k->i1 = k->d;
            AD_makeCheque(k, k->d, 1, _ad->t1055);
            break;
        case 2:
			d = k->d;
			r = k->r;
            k1 = K_divide(k, 1);
            k->i2 = d;
			k->i21 = r;
            AD_makeCheque(k, k->r, 2, _ad->t1055);
			if (k1) {
				k1->i2 = d;
				k1->i21 = r;
				AD_makeCheque(k1, k->d, 1, _ad->t1055);
			}
            break;
        case 3:
            AD_makeAnnul(k, 1, 1, _ad->t1055);
            break;
        case 4:
            k2 = K_divide(k, 2);
            AD_makeAnnulReturn(k, 2, 2, _ad->t1055);
			if (k2)
				AD_makeAnnulReturn(k2, 2, 1, _ad->t1055);
            break;
    }
    return 0;
}

int AD_process(K* k) {
    AD_processO(k);
    AD_save();
    return 0;
}


#define REQUIRED_K_MASK 0x39
#define REQUIRED_L_MASK 0x1F
#define ERR_INVALID_VALUE   45

static K* _k = NULL;
static L* _l = NULL;
static P1* _p1 = NULL;
static uint32_t _kMask;
static uint32_t _lMask;
static uint32_t _p1Mask;

static int process_string_value(const char *tag, const char *name, const char *val,
                                size_t max_size,
                                uint32_t *mask,
                                uint32_t mask_value,
                                char **out) {
    size_t len = val != 0 ? strlen(val) : 0;
    if (len == 0) {
        printf("%s/@%s: Пустое значение\n", tag, name);
        return ERR_INVALID_VALUE;
    } else if (len > max_size) {
        printf("%s/@%s: Превышение длины\n", tag, name);
        return ERR_INVALID_VALUE;
    }
    *out = strdup(val);
    *mask |= mask_value;
    
    return 0;
}

static int process_int_value(const char *tag, const char *name, const char *val,
                                int64_t min, int64_t max,
                                uint32_t *mask,
                                uint32_t mask_value,
                                int64_t *out) {
    char *endp;
    int64_t v = strtoll(val, &endp, 10);
    
    if (endp == NULL || *endp != 0) {
        printf("%s/@%s: Ошибка преобразования к целому типу\n", tag, name);
        return ERR_INVALID_VALUE;
    }
    
    if (v < min || v > max) {
        printf("%s/@%s: Недопустимое значение\n", tag, name);
        return ERR_INVALID_VALUE;
    }

    *out = v;
    *mask |= mask_value;
    
    return 0;
}

static int process_currency_value(const char *tag, const char *name,
                                  const char *val,
                                  uint32_t *mask,
                                  uint32_t mask_value,
                                  int64_t *out)
{
    char *endp;
    double v = strtod(val, &endp);

    if (endp == NULL || *endp != 0) {
        printf("%s/@%s: Ошибка преобразования к вещественному типу\n", tag, name);
        return ERR_INVALID_VALUE;
    }

    *out = (int64_t)((v + 0.009) * 100);
    *mask |= mask_value;

    return 0;
}

static int process_phone_value(const char *tag, const char *name,
                                  const char *val,
                                  uint32_t *mask,
                                  uint32_t mask_value,
                                  char **out)
{
    char *s;
    size_t len = strlen(val);
    if (val == 0) {
        printf("%s/@%s: неправильная длина", tag, name);
        return ERR_INVALID_VALUE;
    }
    char f = val[0];
    if ((f == '+' && len > 15) || (f == '8' && len > 11)) {
        printf("%s/@%s: неправильная длина", tag, name);
        return ERR_INVALID_VALUE;
    }


    if (f == '8') {
        s = malloc(len + 2);
        s[0] = '+';
        s[1] = '7';
        memcpy(s + 2, val + 1, len - 1);
        s[len + 1] = 0;
    } else
        s = strdup(val);
    *out = s;
    *mask |= mask_value;

    return 0;
}

static int process_email_value(const char *tag, const char *name,
                               const char *val,
                               uint32_t *mask,
                               uint32_t mask_value,
                               char **out)
{
    int len = strlen(val);
    if (len < 3) {
        printf("%s/@%s: неправильная длина", tag, name);
        return ERR_INVALID_VALUE;
    }

    int at = -1;
    for (int i = 0; i < len; i++) {
        if (val[i] == '@') {
            if (i == 0 || at >= 0) {
                printf("%s/@%s: Неправильное значение", tag, name);
                return ERR_INVALID_VALUE;
            }
        }
    }
    if (at == len - 1)
        printf("%s/@%s: Неправильное значение", tag, name);

    *out = strdup(val);
    *mask |= mask_value;

    return 0;
}

int check_str_len(const char *tag, const char *name, const char *val,
                  size_t len1, size_t len2)
{
    size_t len = strlen(val);
    if (len != len1 && len != len2) {
        printf("%s/@%s: неправильная длина\n", tag, name);
        return ERR_INVALID_VALUE;
    }
    return 0;
}

// обработка XML
int kkt_xml_callback(uint32_t check, int evt, const char *name, const char *val)
{
    int64_t v64;
    int ret;
    switch (evt) {
        case 0:
            break;
        case 1:
            if (strcmp(name, "K") == 0) {
            	if (_k != NULL)
            		K_destroy(_k);
                _k = K_create();
                _kMask = 0;
            } else if (strcmp(name, "L") == 0) {
            	if (_l != NULL)
            		L_destroy(_l);
                _l = L_create();
                _lMask = 0;
            } else if (strcmp(name, "P1") == 0) {
            	if (_p1 != NULL)
            		P1_destroy(_p1);
                _p1 = P1_create();
                _p1Mask = 0;
            }
            break;
        case 2:
            if (strcmp(name, "K") == 0) {
                if (!check) {
                    AD_process(_k);
                } else {
                    const char tags[] = "ODRPHMTE";
                    for (int i = 0, mask = 1; i < ASIZE(tags); i++, mask <<= 1) {
                        bool required = (REQUIRED_K_MASK & mask) != 0;
                        bool present = (_kMask & mask) != 0;

                        if (i == 1)
                            required = _k->o <= 2;

                        if (required && !present)
                            printf("Обязательный атрибут K/@%c отсутствует\n",
                                   tags[i]);
                    }
                    K_destroy(_k);
                }
                _k = NULL;
            } else if (strcmp(name, "L") == 0) {
                if (!check) {
                    K_addL(_k, _l);
                } else {
                    const char tags[] = "SPRTNC";
                    for (int i = 0, mask = 1; i < ASIZE(tags); i++, mask <<= 1) {
                        bool required = (REQUIRED_L_MASK & mask) != 0;
                        bool present = (_lMask & mask) != 0;

                        if (i == 5 && _l->n <= 4)
                            required = true;

                        if (required && !present)
                            printf("Обязательный атрибут L/@%c отсутствует\n",
                                   tags[i]);
                    }
                    L_destroy(_l);
                }
                _l = NULL;
            } else if (strcmp(name, "P1") == 0) {
                if (!check) {
                	AD_setP1(_p1);
                } else
                	P1_destroy(_p1);
               	_p1 = NULL;
            }
            break;
        case 3:
            if (_l != NULL) {
                if (strcmp(name, "S") == 0 &&
                    (ret = process_string_value("L", name, val, 128, &_lMask, 0x1,
                                                &_l->s)) != 0) {
                    return ret;
                } else if (strcmp(name, "P") == 0) {
                    if ((ret = process_int_value("L", name, val, 1, 4,
                                                     &_lMask, 0x02, &v64)) != 0)
                        return ret;
                    _l->p = (uint8_t)v64;
                } else if (strcmp(name, "R") == 0) {
                    if ((ret = process_int_value("L", name, val, 1, 4,
                                                 &_lMask, 0x04, &v64)) != 0)
                        return ret;
                    _l->r = (uint8_t)v64;
                } else if (strcmp(name, "T") == 0) {
                    if ((ret = process_currency_value("L", name, val,
                                                 &_lMask, 0x08, &_l->t)) != 0)
                        return ret;
                } else if (strcmp(name, "N") == 0) {
                    if ((ret = process_int_value("L", name, val, 0, 6,
                                                 &_lMask, 0x10, &v64)) != 0)
                        return ret;
					_l->n = (uint8_t)v64;
                } else if (strcmp(name, "C") == 0) {
                    if ((ret = process_currency_value("L", name, val,
                                                      &_lMask, 0x20, &_l->c)) != 0)
                        return ret;
                }
            } else if (_k != NULL) {
                if (strcmp(name, "O") == 0) {
                    if ((ret = process_int_value("K", name, val, 0, 4,
                                                 &_kMask, 0x1, &v64)) != 0)
                        return ret;
					_k->o = (uint8_t)v64;
                } else if (strcmp(name, "D") == 0) {
                    if ((ret = check_str_len("K", name, val, 13, 14)) != 0 ||
                        (ret = process_int_value("K", name, val, 0, INT64_MAX,
                                                 &_kMask, 0x2, &v64)) != 0)
                        return ret;
                    _k->d = v64;
                } else if (strcmp(name, "R") == 0) {
                    if ((ret = check_str_len("K", name, val, 13, 14)) != 0 ||
                        (ret = process_int_value("K", name, val, 0, INT64_MAX,
                                                 &_kMask, 0x4, &v64)) != 0)
                        return ret;
                    _k->r = v64;
                } else if (strcmp(name, "P") == 0) {
                    if ((ret = check_str_len("K", name, val, 10, 12)) != 0 ||
                        (ret = process_int_value("K", name, val, 0, INT64_MAX,
                                                 &_kMask, 0x8, &v64)) != 0)
                        return ret;
                    _k->p = v64;
                } else if (strcmp(name, "H") == 0 &&
                           (ret = process_phone_value("K", name, val,
                                                      &_kMask, 0x10, &_k->h)) != 0) {
                    return ret;
                } else if (strcmp(name, "M") == 0) {
                    if ((ret = process_int_value("K", name, val, 0, 3,
                                                 &_kMask, 0x20, &v64)) != 0)
                        return ret;
					_k->m = (uint8_t)v64;
                } else if (strcmp(name, "T") == 0 &&
                          (ret = process_phone_value("K", name, val,
                                                     &_kMask, 0x40, &_k->t)) != 0) {
                    return ret;
                }  else if (strcmp(name, "E") == 0 &&
                            (ret = process_email_value("K", name, val,
                                                       &_kMask, 0x80, &_k->e)) != 0) {
                    return ret;
                }
            } else if (_p1 != NULL) {
                if (strcmp(name, "I") == 0) {
                    _p1->i = strdup(val);
                    _p1Mask |= 0x01;
                } else if (strcmp(name, "P") == 0) {
                    _p1->p = strdup(val);
                    _p1Mask |= 0x02;
                } else if (strcmp(name, "T") == 0) {
                    _p1->t = strdup(val);
                    _p1Mask |= 0x04;
                } else if (strcmp(name, "C") == 0) {
                    _p1->c = strdup(val);
                    _p1Mask |= 0x08;
                }
            }
            break;
        case 4:
			if (!check)
				AD_calc_sum();
            break;
    }
    return 0;
}


void AD_calc_sum() {
	memset(_ad->sum, 0, sizeof(_ad->sum));
	for (list_item_t *li1 = _ad->clist.head; li1 != NULL; li1 = li1->next) {
		C *c = LIST_ITEM(li1, C);
		memset(&c->sum, 0, sizeof(c->sum));
		for (list_item_t *li2 = c->klist.head; li2 != NULL; li2 = li2->next) {
			K *k = LIST_ITEM(li2, K);
			for (list_item_t *li3 = k->llist.head; li3 != NULL; li3 = li3->next) {
				L *l = LIST_ITEM(li3, L);
				S *s = &_ad->sum[l->p - 1];
				switch (k->m) {
				case 1:
					c->sum.n += l->t;
					s->n += l->t;
					break;
				case 2:
					c->sum.e= l->t;
					s->e += l->t;
					break;
				case 3:
					c->sum.p += l->t;
					s->p += l->t;
					break;
				}
				c->sum.a += l->t;
				s->a += l->t;
			}
		}
	}
}