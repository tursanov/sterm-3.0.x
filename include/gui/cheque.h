#ifndef CHEQUE_H
#define CHEQUE_H

#include "kkt/fd/ad.h"

int cheque_init(void);
void cheque_release(void);
int cheque_draw(void);
bool cheque_execute(void);
void cheque_sync_first(void);

#endif // CHEQUE_H
