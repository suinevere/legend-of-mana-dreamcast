/*----------------------
 | card_store_kos.h
 | Description: The KallistiOS VMU store card_vmu.c keeps PS1 card files in, and its boot hook.
 | Author: suinevere
 | Dependencies: card_store.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_CARD_STORE_KOS_H
#define LOM_CARD_STORE_KOS_H

#include "card_store.h"

extern const card_store_ops card_store_kos;
void card_store_kos_init(void);
int card_store_kos_lcds(void);
int card_store_kos_busy(void);

#endif
