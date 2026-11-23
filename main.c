#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/apic.h>
#include <asm/io.h>
#include <linux/syscalls.h>

#include "lib/include/scth.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("piccionalex");
MODULE_DESCRIPTION("Implementation of the Blocking Queuing Service module");


#define MODULE_NAME "BQS"

unsigned long syscall_table_address = 0x0;
module_param(syscall_table_address, ulong, 0660);


#define MAX_FREE 15
static int free_entries[MAX_FREE];
static int nr_free;
module_param_array(free_entries, int, &nr_free, 0660);

unsigned long new_syscall_array[] = {0x0, 0x0};
#define HACKED_ENTRIES (int)(sizeof(new_syscall_array)/sizeof(unsigned long))


#define AUDIT if(1)

asmlinkage int sys_goto_sleep(const struct pt_regs *regs){
    AUDIT{
        printk(KERN_INFO "%s: Ok questo è un messaggio kernel dalla syscall goto_sleep\n", MODULE_NAME);
    }

    return 0;
} 

asmlinkage int sys_awake(const struct pt_regs * regs){
    AUDIT{
        printk(KERN_INFO "%s: OK questo è un messaggio kernel dalla syscall awake\n", MODULE_NAME);
    }

    return 0;
}


static int __init init_BQS(void){

    int i, ret;

    new_syscall_array[0] = (unsigned long)sys_goto_sleep;
    new_syscall_array[1] = (unsigned long)sys_awake;

    ret = sct_install(syscall_table_address, free_entries, nr_free, new_syscall_array, HACKED_ENTRIES);

    if (ret)
        return ret;

    for(i = 0; i < HACKED_ENTRIES; i++){
        printk(KERN_INFO "%s: syscall_table[%d] hacked with 0x%lx\n", MODULE_NAME, free_entries[i], new_syscall_array[i]);
    }

    return 0;
}

static void __exit exit_BQS(void){
    
    sct_restore();

    printk(KERN_INFO "%s: module unloaded successfully\n", MODULE_NAME);

}


module_init(init_BQS);
module_exit(exit_BQS);

