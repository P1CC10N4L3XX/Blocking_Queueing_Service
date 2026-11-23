P=/sys/module/the_usctm/parameters

sudo insmod BQS.ko \
    syscall_table_address=$(sudo cat $P/sys_call_table_address) \
    free_entries=$(sudo cat $P/free_entries) 