#ifndef DEBUG_H
#define DEBUG_H

#include "e13.h"

void dadd(address name, address type, word param);
void padd(char* s, word len);

void dump_stack(void);
void dump_pent_s(address i, word length);
address dump_pent(address i);
void dump_pool(void);
address dump_dent(address i);
void dump_dict();
void dump_sysvars();

#endif
