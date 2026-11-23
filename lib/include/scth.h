#ifndef SCTH_H
#define SCTH_H

#define SCT_MAX_HACKED 15

int sct_install(unsigned long sct_address, const int *entries, int nr_entries, const unsigned long *funcs, int nr_funcs);
void sct_restore(void);
int sct_index(int i);

#endif