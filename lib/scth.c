#include <linux/kernel.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/irqflags.h>
#include "./include/scth.h"

static unsigned long *sct;
static unsigned long ni_syscall;
static int restore[SCT_MAX_HACKED] = {[0 ... SCT_MAX_HACKED-1] = -1};
static int installed;

static inline void unprotect_memory(void)
{
	unsigned long cr0;

	asm volatile("mov %%cr0, %0" : "=r"(cr0));
	cr0 &= ~0x10000UL;
	asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
}

static inline void protect_memory(void)
{
	unsigned long cr0;

	asm volatile("mov %%cr0, %0" : "=r"(cr0));
	cr0 |= 0x10000UL;
	asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
}

static void write_sct_entry(int idx, unsigned long val)
{
	unsigned long flags;

	local_irq_save(flags);
	unprotect_memory();
	sct[idx] = val;
	protect_memory();
	local_irq_restore(flags);
}

int sct_install(unsigned long sct_address, const int *entries, int nr_entries, const unsigned long *funcs, int nr_funcs)
{
    int idx[SCT_MAX_HACKED];
    int i, j = 0;

    if(!sct_address || nr_entries < 1 || nr_funcs > SCT_MAX_HACKED)
        return -EINVAL;
    if(installed)
        return -EBUSY;

    sct = (unsigned long *) sct_address;

    ni_syscall = sct[entries[nr_entries - 1]];

    for(i = 0; i < nr_entries && j < nr_funcs; i++){
        if(entries[i] < 0 || sct[entries[i]] != ni_syscall)
            continue;
        idx[j++] = entries[i];
    }

    if (j < nr_funcs)
        return -EBUSY;


    for(i = 0; i < nr_funcs; i++){
        restore[i] = idx[i];
        write_sct_entry(idx[i], funcs[i]);
    }
    installed = 1;
    return 0;
}


void sct_restore(void){
    int i;

    for(i = 0; i < SCT_MAX_HACKED; i++){
        if(restore[i] != -1){
            write_sct_entry(restore[i], ni_syscall);
            restore[i] = -1;
        }
    }

    installed = 0;

}

int sct_index(int i){
    return (i > 0 && i < SCT_MAX_HACKED) ? restore[i] : -1;
}
