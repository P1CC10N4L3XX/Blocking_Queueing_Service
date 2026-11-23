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

typedef struct _node{
    struct task_struct *task;
    int pid;
    int awake;
    int already_hit;
    struct _node *next;
    struct _node *prev;
}node_t;


node_t head = {NULL, -1, -1, -1, NULL, NULL};
node_t tail = {NULL, -1, -1, -1, NULL, NULL};
spinlock_t queue_lock;




asmlinkage long sys_goto_sleep(const struct pt_regs *regs){
    volatile node_t me;
    node_t *aux;

    DECLARE_WAIT_QUEUE_HEAD(the_queue);

    me.next = NULL;
    me.prev = NULL;
    me.task = current;
    me.pid = current->pid;
    me.awake = 0;
    me.already_hit = 0;

    AUDIT
    printk(KERN_INFO "%s: sys_goto_sleep called by thread %d\n", MODULE_NAME, me.pid);

    preempt_disable();
    spin_lock(&queue_lock);

    aux = &tail;
    if(aux->prev == NULL){
        spin_unlock(&queue_lock);
        preempt_enable();
        printk(KERN_ERR "%s: malformed sleep-wakeup-queue; service damaged\n", MODULE_NAME);
        return -1;
    }

    aux->prev->next = &me;
    me.prev = aux->prev;
    aux->prev = &me;
    me.next = aux;

    spin_unlock(&queue_lock);
    preempt_enable();

    AUDIT
    printk(KERN_INFO "%s: thread %d is going to sleep\n", MODULE_NAME, me.pid);
    
    wait_event_interruptible(the_queue, me.awake);
    preempt_disable();
    spin_lock(&queue_lock);

    me.prev->next = me.next;
    me.next->prev = me.prev;

    spin_unlock(&queue_lock);
    preempt_enable();

    AUDIT
    printk(KERN_INFO "%s: thread %d woke up\n", MODULE_NAME, me.pid);

    if(me.awake == 0){
        AUDIT
        printk(KERN_INFO "%s: thread %d wake up for signal\n", MODULE_NAME, me.pid);
        return -EINTR;
    }

    return 0;
} 

asmlinkage long sys_awake(const struct pt_regs * regs){
    struct task_struct *the_task;
    int its_pid = -1;
    node_t *aux;

    AUDIT
    printk("%s: sys_awake called by thread %d\n", MODULE_NAME, current->pid);

    aux=&head;

    preempt_disable();
    spin_lock(&queue_lock);

    if(aux==NULL){
        spin_unlock(&queue_lock);
        preempt_enable();
        printk(KERN_ERR "%s: malformed sleep-wakeup-queue; service damaged\n", MODULE_NAME);
        return -1;
    }

    while(aux->next != NULL){
        AUDIT
        printk(KERN_INFO "%s: in cycle\n", MODULE_NAME);
        if(aux->next->already_hit == 0){
            the_task = aux->next->task;
            aux->next->awake = 1;
            aux->next->already_hit = 1;
            its_pid = aux->next->pid;
            wake_up_process(the_task);
            spin_unlock(&queue_lock);
            preempt_enable();

            AUDIT 
            printk(KERN_INFO "%s: called the awake of thread %d\n", MODULE_NAME, its_pid);
            
            return its_pid;
        }
    }

    spin_unlock(&queue_lock);
    preempt_enable();

    return 0;
}


static int __init init_BQS(void){

    int i, ret;

    new_syscall_array[0] = (unsigned long)sys_goto_sleep;
    new_syscall_array[1] = (unsigned long)sys_awake;

    ret = sct_install(syscall_table_address, free_entries, nr_free, new_syscall_array, HACKED_ENTRIES);

    if (ret)
        return ret;

    spin_lock_init(&queue_lock);

    head.next = &tail;
    tail.prev = &head;


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

