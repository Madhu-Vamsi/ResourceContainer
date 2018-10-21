#include "processor_container.h"

#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/sched.h>

extern struct miscdevice processor_container_dev;


struct mutex lock;
/**
 * Initialize and register the kernel module
 */
int processor_container_init(void)
{
    int ret;
    if ((ret = misc_register(&processor_container_dev)))
        printk(KERN_ERR "Unable to register \"processor_container\" misc device\n");
    else
        printk(KERN_ERR "\"processor_container\" misc device installed\n");
    mutex_init(&lock);
    return ret;
}


/**
 * Cleanup and deregister the kernel module
 */ 
void processor_container_exit(void)
{
    misc_deregister(&processor_container_dev);
}
