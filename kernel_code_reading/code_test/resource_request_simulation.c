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
#include "resource_case.h"
#include "resource_request_simulation.h"

struct pci_bus *root_bus;
struct resource root_res[3];

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
	pci_hierachy_setup();
	random_resource_size();

	dump_pci_hierachy(root_bus, 0, true);
}
