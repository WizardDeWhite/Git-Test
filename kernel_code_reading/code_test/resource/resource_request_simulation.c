/*
 * =====================================================================================
 *
 *       Filename:  resource_request_simulation.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/29/2013 10:56:07 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wei Yang (weiyang), weiyang.kernel@gmail.com
 *        Company:  
 *
 * =====================================================================================
 */
#include <time.h>
#include <stdlib.h>
#include "resource.h"
#include "resource_request_simulation.h"

struct pci_bus *root_bus;
struct resource root_res[3];

void dump2(struct resource *root, int level)
{
	struct resource *tmp    = NULL;

	if (!root)
		return;

	/* print itself first */
	printf("%*c%08lx-%08lx  %s\n",level*3, ' ', root->start, root->end,
			root->flags|IORESOURCE_BUSY?"busy":"none");

	/* depth first */
	dump2(root->child, level+1);

	/* then brothers */
	dump2(root->sibling, level);

	return;
}

struct pci_dev* create_pci_dev(struct pci_bus *p_bus, 
		struct pci_dev *p_dev,
		int index)
{
	struct pci_dev* dev;

	dev = (struct pci_dev*)malloc(sizeof(*dev));

	dev->parent = p_dev;
	dev->subordinate = NULL;
	snprintf(dev->name, sizeof(dev->name), "%s.%d",
			p_bus->name, index);
	INIT_LIST_HEAD(&dev->bus_list);

	/* link to the parent bus */
	list_add_tail(&dev->bus_list, &p_bus->devices);

	return dev;
}

struct pci_bus* create_pci_bus(struct pci_bus *p_bus, int num)
{
	struct pci_bus* bus;

	bus = (struct pci_bus*)malloc(sizeof(*bus));

	INIT_LIST_HEAD(&bus->node);
	INIT_LIST_HEAD(&bus->children);
	INIT_LIST_HEAD(&bus->devices);
	bus->parent = p_bus;
	bus->number = num;
	snprintf(bus->name, sizeof(bus->name), "0000:%d", bus->number);

	/* link to the parent bus */
	if (p_bus)
		list_add_tail(&bus->node, &p_bus->children);

	return bus;
}
                      
void dump_pci_hierachy(struct pci_bus *bus, int level, bool with_res)
{
	struct pci_dev *pdev;
	/* print the bus information */
	printf("%*c BUS %s\n", level*3, ' ', bus->name);

	/* print its children */
	list_for_each_entry(pdev, &bus->devices, bus_list) {
		printf("%*c* DEV %s\n", level*3+2, ' ', pdev->name);
		if (with_res) {
			printf("%*c      res[0] %05llx-%05llx\n", level*3+2, ' ', 
				(unsigned long long)pdev->resource[0].start, 
				(unsigned long long)pdev->resource[0].end);
			printf("%*c      res[1] %08llx-%08llx\n", level*3+2, ' ', 
				(unsigned long long)pdev->resource[1].start, 
				(unsigned long long)pdev->resource[1].end);
			printf("%*c      res[2] %08llx-%08llx\n", level*3+2, ' ', 
				(unsigned long long)pdev->resource[2].start, 
				(unsigned long long)pdev->resource[2].end);
		}

		if (pdev->subordinate)
			dump_pci_hierachy(pdev->subordinate, level+1, with_res);
	}
}

/*                               
 *                               
 *                               
 *                          
 *                         Bus 0 
 *          -------+-------+------------------+-----
 *                 | 0000:0.0                 | 0000:0.1
 *           +-----+----+                +----+----+
 *           |          |                |         |
 *           +----------+                +----+----+
 *                                            |
 *                                            | Bus 1
 *                 ---+---------------+-------+-----------+------
 *                    |0000:1.0       |0000:1.1           |0000:1.2
 *                +---+----+      +---+----+           +--+----+
 *                |        |      |        |           |       |
 *                +---+----+      +--------+           +-------+
 *                    |  Bus 2
 *        --+---------+---------+--
 *          |0000:2.0           |0000:2.1
 *     +----+-----+       +-----+----+
 *     |          |       |          |
 *     +----------+       +----------+
 *
 *
 * */

int pci_hierachy_setup()
{
	struct pci_bus *bus;
	struct pci_dev *dev;
	int bus_num = 0;
	int dev_num = 0;

	root_bus = create_pci_bus(NULL, bus_num++);
	bus = root_bus;

	/* create two device under root bus */
	create_pci_dev(root_bus, NULL, dev_num++);
	dev = create_pci_dev(root_bus, NULL, dev_num++);

	/* one bus below the second the device */
	bus = create_pci_bus(root_bus, bus_num++);
	dev->subordinate = bus;
	/* and there are three devices under the bus */
	dev_num = 0;
	create_pci_dev(bus, dev, dev_num++);
	create_pci_dev(bus, dev, dev_num++);
	create_pci_dev(bus, dev, dev_num++);

	/* one bus below the first device */
	dev = list_first_entry(&bus->devices, struct pci_dev, bus_list);
	bus = create_pci_bus(bus, bus_num++);
	dev->subordinate = bus;
	/* two devices of this bus */
	dev_num = 0;
	create_pci_dev(bus, dev, dev_num++);
	create_pci_dev(bus, dev, dev_num++);
	
	return 0;
}

/*
 *
 *
 *
 *                         Bus 0
 *          -------+-------+------------------+-----
 *                 | 0000:0.0                 | 0000:0.1
 *           +-----+----+                +----+----+
 *           |          |                |         |
 *           +----+-----+                +----+----+
 *                |                            
 *                |Bus 1
 *           -----+---+---------------+-------------------+------
 *                    |0000:1.0       |0000:1.1           |0000:1.2
 *                +---+----+      +---+----+           +--+----+
 *                |        |      |        |           |       |
 *                +---+----+      +--------+           +-------+
 *                    |  Bus 2
 *        --+---------+---------+--
 *          |0000:2.0           |0000:2.1
 *     +----+-----+       +-----+----+
 *     |          |       |          |
 *     +----------+       +----------+
 *
 *
 * */

int pci_hierachy_setup1()
{
	struct pci_bus *bus;
	struct pci_dev *dev;
	int bus_num = 0;
	int dev_num = 0;

	root_bus = create_pci_bus(NULL, bus_num++);
	bus = root_bus;

	/* create two device under root bus */
	dev = create_pci_dev(root_bus, NULL, dev_num++);
	create_pci_dev(root_bus, NULL, dev_num++);

	/* one bus below the second the device */
	bus = create_pci_bus(root_bus, bus_num++);
	dev->subordinate = bus;
	/* and there are three devices under the bus */
	dev_num = 0;
	create_pci_dev(bus, dev, dev_num++);
	create_pci_dev(bus, dev, dev_num++);
	create_pci_dev(bus, dev, dev_num++);

	/* one bus below the first device */
	dev = list_first_entry(&bus->devices, struct pci_dev, bus_list);
	bus = create_pci_bus(bus, bus_num++);
	dev->subordinate = bus;
	/* two devices of this bus */
	dev_num = 0;
	create_pci_dev(bus, dev, dev_num++);
	create_pci_dev(bus, dev, dev_num++);

	return 0;
}

/*
 *
 *
 *
 *                         Bus 0
 *          -------+-------+-----
 *                 | 0000:0.0
 *           +-----+----+ 
 *           |          |
 *           +----+-----+
 *                |
 *                |Bus 1
 *           -----+---+----------
 *                    |0000:1.0 
 *                +---+----+   
 *                |        |  
 *                +---+----+  
 *                    |  Bus 2
 *        --+---------+---------+--
 *          |0000:2.0           |0000:2.1
 *     +----+-----+       +-----+----+
 *     |          |       |          |
 *     +----------+       +----------+
 *
 *
 * */

int pci_hierachy_setup2()
{
	struct pci_bus *bus;
	struct pci_dev *dev;
	int bus_num = 0;
	int dev_num = 0;

	root_bus = create_pci_bus(NULL, bus_num++);
	bus = root_bus;

	/* create one device under root bus */
	dev = create_pci_dev(root_bus, NULL, dev_num++);

	/* one bus below the device */
	bus = create_pci_bus(root_bus, bus_num++);
	dev->subordinate = bus;
	/* and there are one device under the bus */
	dev_num = 0;
	create_pci_dev(bus, dev, dev_num++);

	/* one bus below the first device */
	dev = list_first_entry(&bus->devices, struct pci_dev, bus_list);
	bus = create_pci_bus(bus, bus_num++);
	dev->subordinate = bus;
	/* two devices of this bus */
	dev_num = 0;
	create_pci_dev(bus, dev, dev_num++);
	create_pci_dev(bus, dev, dev_num++);

	return 0;
}

/*
 *
 *
 *
 *                         Bus 0
 *          -------+-------+------------------+-----
 *                 | 0000:0.0                 | 0000:0.1
 *           +-----+----+                +----+----+
 *           |          |                |         |
 *           +----+-----+                +----+----+
 *                |                            
 *                |Bus 1
 *           -----+---+---------------+-------------------+------
 *                    |0000:1.0       |0000:1.1           |0000:1.2
 *                +---+----+      +---+----+           +--+----+
 *                |        |      |        |           |       |
 *                +---+----+      +--------+           +--+----+
 *                    |  Bus 2                            |Bus 3
 *        --+---------+---------+--               --------+----------------
 *          |0000:2.0           |0000:2.1                 |0000:3.0
 *     +----+-----+       +-----+----+            +-------+--------+
 *     |          |       |          |            |                |
 *     +----------+       +----------+            +-------+--------+
 *                                                        |
 *                                                        | Bus 4
 *                                                 -------+---------------
 *                                                        |0000:4.0
 *                                                 +------+-------+
 *                                                 |              |
 *                                                 +------+-------+
 *                                                        |
 *                                                        | Bus 5
 *                                                 -------+--------------
 *                                                        |0000:5.0
 *                                                 +------+-------+
 *                                                 |              |
 *                                                 +--------------+
 *
 *
 * */

int pci_hierachy_setup3()
{
	struct pci_bus *bus, *bus1;
	struct pci_dev *dev, *dev1;
	int bus_num = 0;
	int dev_num = 0;

	root_bus = create_pci_bus(NULL, bus_num++);
	bus = root_bus;

	/* create two device under root bus */
	dev = create_pci_dev(root_bus, NULL, dev_num++);
	create_pci_dev(root_bus, NULL, dev_num++);

	/* one bus below the second the device */
	bus = create_pci_bus(root_bus, bus_num++);
	dev->subordinate = bus;
	/* and there are three devices under the bus */
	dev_num = 0;
	create_pci_dev(bus, dev, dev_num++);
	create_pci_dev(bus, dev, dev_num++);
	dev1 = create_pci_dev(bus, dev, dev_num++);
	bus1 = bus;

	/* one bus below the first device */
	dev = list_first_entry(&bus->devices, struct pci_dev, bus_list);
	bus = create_pci_bus(bus, bus_num++);
	dev->subordinate = bus;
	/* two devices of this bus */
	dev_num = 0;
	create_pci_dev(bus, dev, dev_num++);
	create_pci_dev(bus, dev, dev_num++);

	/* one bus under dev 0000:1.2 */
	bus = create_pci_bus(bus1, bus_num++);
	dev1->subordinate = bus;
	/* one devices of this bus */
	dev_num = 0;
	dev = create_pci_dev(bus, dev1, dev_num++);

	/* one bus under this dev */
	bus = create_pci_bus(bus, bus_num++);
	dev->subordinate = bus;
	/* one devices of this bus */
	dev_num = 0;
	dev = create_pci_dev(bus, dev, dev_num++);

	/* one bus under this dev */
	bus = create_pci_bus(bus, bus_num++);
	dev->subordinate = bus;
	/* one devices of this bus */
	dev_num = 0;
	dev = create_pci_dev(bus, dev, dev_num++);

	return 0;
}

void __random_resource_size(struct pci_bus *bus)
{
	struct pci_dev *pdev;

	list_for_each_entry(pdev, &bus->devices, bus_list) {
		pdev->resource[0].flags = IORESOURCE_IO;
		pdev->resource[0].start = 0;
		pdev->resource[0].end = 
			(rand() % root_res[0].end + 1) / 4;

		pdev->resource[1].flags = IORESOURCE_MEM;
		pdev->resource[1].start = 0;
		pdev->resource[1].end = 
			(rand() % root_res[1].end + 1) / 4;

		pdev->resource[2].flags = IORESOURCE_MEM | IORESOURCE_PREFETCH;
		pdev->resource[2].start = 0;
		pdev->resource[2].end = 
			(rand() % root_res[2].end + 1) / 4;

		if (pdev->subordinate)
			__random_resource_size(pdev->subordinate);

	}
}

void random_resource_size()
{
	struct pci_dev *pdev;

	srand(time(NULL));

	root_res[0].flags = IORESOURCE_IO;
	root_res[0].start = 0;
	root_res[0].end   = 0xfffff; // 1M

	root_res[1].flags = IORESOURCE_MEM;
	root_res[1].start = 0;
	root_res[1].end   = 0x3fffffff; // 1G

	root_res[2].flags = IORESOURCE_MEM | IORESOURCE_PREFETCH;
	root_res[2].start = 0;
	root_res[2].end   = 0xffffffff; // 4G

	__random_resource_size(root_bus);
}

void pci_init()
{
	pci_hierachy_setup3();
	random_resource_size();

	dump_pci_hierachy(root_bus, 0, false);
}

struct pci_depth* pci_depth_setup(struct pci_bus *bus, int dep)
{
	struct pci_depth *bus_node;
	if (!bus)
		return NULL;

	bus_node = (struct pci_depth*)malloc(sizeof(*bus_node));
	INIT_LIST_HEAD(&bus_node->node);

	bus_node->bus = bus;
	bus_node->dep = dep;

	return bus_node;
}

void pci_depth_release(struct pci_depth *bus_node)
{
	free(bus_node);
}

int pci_bus_get_depth2(struct pci_bus *bus)
{
	int max_depth, depth;
	struct pci_bus *parent, *curr;
	struct list_head *node;

	/* no child? */
	if (list_empty(&bus->children))
		return 0;

	node = bus->children.next;
	parent = bus;
	max_depth = depth = 1;

	while(parent) {
		/* hit the head, go back to parent level */
		if (node == &parent->children) {
			node = parent->node.next;
			parent = parent->parent;
			depth--;
			continue;
		}
		curr = list_entry(node, struct pci_bus, node);
		/* depth first */
		if (!list_empty(&curr->children)) {
			node = curr->children.next;
			parent = curr;
			depth++;
			if (max_depth < depth)
				max_depth = depth;
		}
		/* no child, go to the sibling */
		else
			node = curr->node.next;
	}

	return max_depth;
}

int pci_bus_get_depth1(struct pci_bus *bus)
{
	int depth = 0;
	struct pci_bus *child_bus;
	struct pci_depth *bus_node, *bus_node1;
	LIST_HEAD(buses);

	if (!bus)
		return 0;

	bus_node = pci_depth_setup(root_bus, 0);

	list_add_tail(&bus_node->node, &buses);

	while(!list_empty(&buses)) {
		bus_node = list_first_entry(&buses, struct pci_depth, node);
		list_del(&bus_node->node);

		if (depth < bus_node->dep)
			depth = bus_node->dep;

		list_for_each_entry(child_bus, &bus_node->bus->children, node) {
			bus_node1 = pci_depth_setup(child_bus, bus_node->dep + 1);

			list_add_tail(&bus_node1->node, &buses);
		}

		pci_depth_release(bus_node);
	}

	return depth;
}

int pci_bus_get_depth(struct pci_bus *bus)
{
	int depth = 0;
	struct pci_bus *child_bus;

	list_for_each_entry(child_bus, &bus->children, node){
		int ret;

		ret = pci_bus_get_depth(child_bus);
		if (ret + 1 > depth)
			depth = ret + 1;
	}

	return depth;
}


resource_size_t calculate_mem_align(resource_size_t *aligns,
						  int max_order)
{
	resource_size_t align = 0;
	resource_size_t min_align = 0;
	int order;

	for (order = 0; order <= max_order; order++) {
		resource_size_t align1 = 1;

		align1 <<= (order + 20);

		if (!align)
			min_align = align1;
		else if (ALIGN(align + min_align, min_align) < align1)
			min_align = align1 >> 1;
		align += aligns[order];
	}

	return min_align;
}
