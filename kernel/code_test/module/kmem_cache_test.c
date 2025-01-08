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

struct typesafe_obj {
	refcount_t refcnt;
};

static void typesafe_init(struct typesafe_obj *obj)
{
	memset(obj, 0, sizeof(*obj));
}

static int do_kmem_cache_typesafe(int *total_failures)
{
	struct kmem_cache *c;
	struct typesafe_obj *buf, *buf_new;
	int oldcnt;
	bool fail = false;

	c = kmem_cache_create("test_cache", sizeof(struct typesafe_obj),
			sizeof(struct typesafe_obj), SLAB_TYPESAFE_BY_RCU, NULL);
	/* allocate from typesafe slab */
	buf = kmem_cache_alloc(c, GFP_KERNEL);
	/* do initialization immediately */
	typesafe_init(buf);
	pr_info("buf(%p)->refcnt: %d\n", buf, refcount_read(&buf->refcnt));
	/* increase refcnt before freeing */
	refcount_set(&buf->refcnt, 1);
	oldcnt = refcount_read(&buf->refcnt);
	pr_info("buf(%p)->refcnt: %d\n", buf, oldcnt);

	/* rcu read critical section */
	rcu_read_lock();

	kmem_cache_free(c, buf);

	/* just allocate, defer the initialization */
	buf_new = kmem_cache_alloc(c, GFP_KERNEL);
	pr_info("buf_new(%p)->refcnt: %d\n", buf_new, oldcnt);

	/* expect to get the same object */
	if (buf != buf_new)
		fail = true;

	/* expect refcnt not change, since defer initialization */
	if (oldcnt != refcount_read(&buf_new->refcnt))
		fail = true;

	/* now do the initialization */
	typesafe_init(buf_new);
	pr_info("buf_new(%p)->refcnt: %d after initialization\n",
			buf_new, refcount_read(&buf_new->refcnt));
	/* both buf|buf_new->refcnt should be 0 */
	if (0 != refcount_read(&buf_new->refcnt))
		fail = true;
	if (0 != refcount_read(&buf->refcnt))
		fail = true;

	/* rcu read critical section */
	rcu_read_unlock();

	/* free the new obj */
	kmem_cache_free(c, buf_new);
	kmem_cache_destroy(c);
	*total_failures += fail;
	return 1;
}

static int test_typesafe(int *total_failures)
{
	int failures = 0, num_tests = 0;

	num_tests += do_kmem_cache_typesafe(&failures);

	*total_failures += failures;
	return num_tests;
}

static int kmem_cache_test_init(void)
{
	int failures = 0, num_tests = 0;

	num_tests += test_typesafe(&failures);

	if (failures == 0)
		pr_info("all %d tests passed!\n", num_tests);
	else
		pr_info("failures: %d out of %d\n", failures, num_tests);

	return -1;
}
static void kmem_cache_test_exit(void)
{
        printk(KERN_ALERT "Goodbye, cruel world\n");
}
module_init(kmem_cache_test_init);
module_exit(kmem_cache_test_exit);
