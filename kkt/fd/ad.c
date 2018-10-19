#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "kkt/fd/ad.h"

static int save_string(FILE *f, char *s) {
    size_t len = s != NULL ? strlen(s) : 0;
    if (fwrite(&len, sizeof(len), 1, f) < 0)
        return -1;
    if (fwrite(s, len, 1, f) < 0)
        return -1;
    return 0;
}

static int load_string(FILE *f, char **s) {
    size_t len;
    if (fread(&len, sizeof(len), 1, f) < 0 || len == 0)
        return -1;
    *s = (char *)malloc(len + 1);
    if (*s == NULL)
        return -1;
    
    if (fread(*s, len, 1, f) < 0) {
        free(*s);
        return -1;
    }
    *s[len] = 0;
    
    return 0;
}

static int save_int(FILE *f, uint64_t v, size_t size) {
    if (fwrite(&v, size, 1, f) < 0)
        return -1;
    return 0;
}
#define SAVE_INT(f, v) save_int((f), (v), sizeof(v))

static int load_int(FILE *f, uint64_t v, size_t size) {
    if (fread(&v, size, 1, f) < 0)
        return -1;
    return 0;
}
#define LOAD_INT(f, v) load_int((f), (v), sizeof(v))

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

int L_save(L *l, FILE *f) {
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
    return list_add(&k->llist, l);
}

#define COPYSTR(s) ((s) != NULL ? strdup(s) : NULL)

struct K_divide_arg {
    list_t *llist;
    uint8_t p;
};

static bool K_divide_func(struct K_divide_arg *arg, L *l) {
    if (l->p == arg->p) {
        list_add(arg->llist, l);
        return true;
    }
    return false;
}

K *K_divide(K *k, uint8_t p) {
    K *k1 = (K*)malloc(sizeof(K));
    struct K_divide_arg arg = { &k1->llist, p };
    
    k1->o = k->o;          // Операция
    k1->d = k->d;          // Номер оформляемого документа или КРС при возврате
    k1->r = k->r;          // Номер документа, для которого оформляется дубликат, или возвращаемого документа или гасимого документа или гасимой КРС возврата
    k1->i = k->i;          // индекс
    k1->p = k->p;          // ИНН перевозчика

    k1->h = COPYSTR(k->h); // телефон перевозчика
    k1->m = k->m;          // способ оплаты
    k1->t = COPYSTR(k->t);            // номер телефона пассажира
    k1->e = COPYSTR(k->e);            // адрес электронной посты пассажира
 
    // разделяем на 2 части
    list_remove_if(&k->llist, &arg, (list_item_func_t)K_divide_func);
 
    return k;
}

// сравнение строк (включает сравнение на NULL)
static bool strcmpex(const char *s1, const char *s2) {
    if (s1 == NULL)
        return s2 == NULL;
    else if (s2 == NULL)
        return s1 == NULL;
    return strcmp(s1, s2) == 0;
}

static bool L_compare(L *l1, L *l2) {
    return (strcmpex(l1->s, l2->s) &&
            l1->r == l2->r &&
            l1->t == l2->t &&
            l1->n == l2->n &&
            l1->c == l2->c);
}

bool K_equalByL(K *k1, K* k2) {
    return list_compare(&k1->llist, &k2->llist, NULL, (list_item_compare_func_t)L_compare) == 0;
}

int K_save(K *k, FILE *f) {
    if (save_list(f, &k->llist, (list_item_func_t)L_save) < 0 ||
        SAVE_INT(f, k->o) < 0 ||
        SAVE_INT(f, k->d) < 0 ||
        SAVE_INT(f, k->r) < 0 ||
        SAVE_INT(f, k->i) < 0 ||
        SAVE_INT(f, k->p) < 0 ||
        save_string(f, k->h) < 0 ||
        SAVE_INT(f, k->m) < 0 ||
        save_string(f, k->t) < 0 ||
        save_string(f, k->e) < 0)
        return -1;
    return 0;
}

K *K_load(FILE *f) {
    K *k = K_create();
    if (load_list(f, &k->llist, (load_item_func_t)L_load) < 0 ||
            LOAD_INT(f, k->o) < 0 ||
            LOAD_INT(f, k->d) < 0 ||
            LOAD_INT(f, k->r) < 0 ||
            LOAD_INT(f, k->i) < 0 ||
            LOAD_INT(f, k->p) < 0 ||
            load_string(f, &k->h) < 0 ||
            LOAD_INT(f, k->m) < 0 ||
            load_string(f, &k->t) < 0 ||
            load_string(f, &k->e) < 0) {
        K_destroy(k);
        return NULL;
    }
    return k;
}

void S_add(S *dst, S*src) {
    dst->a += src->a;
    dst->n += src->n;
    dst->e += src->e;
    dst->p += src->p;
}

void S_subtract(S *dst, S*src) {
    dst->a -= src->a;
    dst->n -= src->n;
    dst->e -= src->e;
    dst->p -= src->p;
}

void S_addValue(uint8_t m, int64_t value, S *sum, size_t count) {
    for (size_t i = 0; i < count; i++, sum++) {
        sum->a += value;
        switch (m) {
            case 1:
                sum->n += value;
                break;
            case 2:
                sum->e += value;
                break;
            case 3:
                sum->p += value;
                break;
        }
    }
}

void S_subtractValue(uint8_t m, int64_t value, S *sum, size_t count) {
    for (size_t i = 0; i < count; i++, sum++) {
        sum->a -= value;
        switch (m) {
            case 1:
                sum->n -= value;
                break;
            case 2:
                sum->e -= value;
                break;
            case 3:
                sum->p -= value;
                break;
        }
    }
}

static int S_save(S *s, FILE *f) {
    if (SAVE_INT(f, s->a) < 0 ||
        SAVE_INT(f, s->n) < 0 ||
        SAVE_INT(f, s->e) < 0 ||
        SAVE_INT(f, s->p) < 0)
        return -1;
    return 0;
}

static int S_load(S *s, FILE *f) {
    if (LOAD_INT(f, s->a) < 0 ||
        LOAD_INT(f, s->n) < 0 ||
        LOAD_INT(f, s->e) < 0 ||
        LOAD_INT(f, s->p) < 0)
        return -1;
    return 0;

}

C* C_create(void) {
    C *c = (C *)malloc(sizeof(C));
    memset(c, 0, sizeof(C));
    c->klist.delete_func = (list_item_delete_func_t)K_destroy;
    
    return c;
}

void C_destroy(C *c) {
    list_clear(&c->klist);
    if (c->s != NULL)
        free(c->s);
    if (c->pe != NULL)
        free(c->pe);
}

bool C_addK(C *c, K *k) {
    return list_add(&c->klist, k);
}

int C_save(C *c, FILE *f) {
    if (save_list(f, &c->klist, (list_item_func_t)K_save) < 0 ||
            SAVE_INT(f, c->p) < 0 ||
            save_string(f, c->s) < 0 ||
            SAVE_INT(f, c->t1054) < 0 ||
            SAVE_INT(f, c->t1055) < 0 ||
            S_save(&c->sum, f) < 0 ||
            SAVE_INT(f, c->t1086) < 0 ||
            save_string(f, c->pe) < 0)
        return -1;
    return 0;
}

C * C_load(FILE *f) {
    C *c = C_create();
    if (load_list(f, &c->klist, (load_item_func_t)K_load) < 0 ||
            LOAD_INT(f, c->p) < 0 ||
            load_string(f, &c->s) < 0 ||
            LOAD_INT(f, c->t1054) < 0 ||
            LOAD_INT(f, c->t1055) < 0 ||
            S_load(&c->sum, f) < 0 ||
            LOAD_INT(f, c->t1086) < 0 ||
            load_string(f, &c->pe) < 0) {
        C_destroy(c);
        return NULL;
    }
    return c;
}

AD *AD_create(void) {
    AD *ad = (AD *)malloc(sizeof(AD));
    memset(ad, 0, sizeof(AD));
    
    ad->clist.delete_func = (list_item_delete_func_t)C_destroy;
    
    return ad;
}

void AD_destroy(AD *ad) {
    list_clear(&ad->clist);
    if (ad->t1086 != NULL)
        free(ad->t1086);
    free(ad);
}

int AD_save(AD *ad, const char *file_name) {
    int ret;
    FILE *f = fopen(file_name, "wb");
    if (f == NULL)
        return -1;
    
    if (save_string(f, ad->p1.i) < 0 ||
        save_string(f, ad->p1.p) < 0 ||
        save_string(f, ad->p1.t) < 0 ||
        save_string(f, ad->p1.s) < 0 ||
        SAVE_INT(f, ad->t1055) < 0 ||
        S_save(&ad->sum[0], f) < 0 ||
        S_save(&ad->sum[1], f) < 0 ||
        S_save(&ad->sum[2], f) < 0 ||
        S_save(&ad->sum[3], f) < 0 ||
        save_list(f, &ad->clist, (list_item_func_t)C_save) < 0)
        ret = -1;
    else
        ret = 0;
    
    fclose(f);
    return ret;
}

int AD_load(const char *file_name, AD **ad) {
    *ad = AD_create();
    FILE *f = fopen(file_name, "rb");
    if (f == NULL)
        return -1;
    int ret;
    
    if (load_string(f, &(*ad)->p1.i) < 0 ||
        load_string(f, &(*ad)->p1.p) < 0 ||
        load_string(f, &(*ad)->p1.t) < 0 ||
        load_string(f, &(*ad)->p1.s) < 0 ||
        LOAD_INT(f, (*ad)->t1055) < 0 ||
        S_load(&(*ad)->sum[0], f) < 0 ||
        S_load(&(*ad)->sum[1], f) < 0 ||
        S_load(&(*ad)->sum[2], f) < 0 ||
        S_load(&(*ad)->sum[3], f) < 0 ||
        load_list(f, &(*ad)->clist, (load_item_func_t)C_load) < 0)
        ret = -1;
    else
        ret = 0;
    
    fclose(f);
    return ret;
}

void AD_setT1086(AD *ad, const char *i, const char *p, const char *t) {
    if (ad->t1086 != NULL)
        free(ad->t1086);
    
    size_t lenI = i != NULL ? strlen(i) : 0;
    size_t lenP = p != NULL ? strlen(p) : 0;
    size_t lenT = t != NULL ? strlen(t) : 0;
    
    ad->t1086 = (char *)malloc(lenI + lenP + lenT + 1);
    char *ptr = ad->t1086;
    if (lenI > 0) {
        memcpy(ptr, i, lenI);
        ptr += lenI;
    }
    if (lenP > 0) {
        memcpy(ptr, p, lenP);
        ptr += lenP;
    }
    if (lenT > 0) {
        memcpy(ptr, t, lenT);
        ptr += lenT;
    }
    *ptr = 0;
}
