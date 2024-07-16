#include <stdio.h>
#include "test_common.h"
#include "maple_tree.h"

void maple_tree_test()
{
	int i;

	prefix_reset();
	prefix_push("maple_tree_test");
	test_print("Running %s tests...\n", __func__);

	PREFIX_PUSH();

	test_pass_pop();

	prefix_pop();
}

int main(int argc, char *argv[])
{
	parse_args(argc, argv);

	maple_tree_test();

	return 0;
}
