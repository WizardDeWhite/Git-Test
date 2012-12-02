/*
 * =====================================================================================
 *
 *       Filename:  hello_world.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/21/2012 11:38:18 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("Dual BSD/GPL");

static unsigned int num;
module_param(num, uint, S_IRUGO);
MODULE_PARM_DESC(num, "Number used in this example");

static int hello_init(void)
{
        printk(KERN_ALERT "Hello, world %d\n", num);
        return 0;
}
static void hello_exit(void)
{
        printk(KERN_ALERT "Goodbye, cruel world\n");
}
module_init(hello_init);
module_exit(hello_exit);
