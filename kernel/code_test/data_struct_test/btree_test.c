#include <stdio.h>
#include "btree.h"

void getidx_in_node()
{
	int index;
	struct btree_node node;

	node.key[0] = 22;
	node.key[1] = 33;
	node.used = 2;

	if (get_idx(&node, 22, &index))
		printf("key %d at index %d\n", 22, index);
	if (!get_idx(&node, 23, &index))
		printf("key %d not exist before %d\n", 23, index);
	if (!get_idx(&node, 43, &index))
		printf("key %d not exist before %d\n", 43, index);

	node.key[2] = 42;
	node.key[3] = 53;
	node.used = 4;
	if (!get_idx(&node, 43, &index))
		printf("key %d not exist before %d\n", 43, index);
	if (get_idx(&node, 53, &index))
		printf("key %d at index %d\n", 53, index);
}

void __insert_key(struct btree *tree, int *key, int len);
void lookup_key()
{
	struct btree tree = BTREE;
	struct btree_node *node;
	int key[] = {22, 33, 10, 15};
	int index;
	void *data;

	data = btree_lookup(&tree, key[0]);
	if (data)
		panic("We shouldn't find %d in empty tree\n", key[0]);
#ifdef DEBUH
	printf("Not find key %d \n", key[0]);
#endif

	__insert_key(&tree, key, ARRAY_SIZE(key));
	data = btree_lookup(&tree, key[0]);
	if (!data)
		panic("We failed to find %d in tree\n", key[0]);
	if (data != &key[0])
		panic("We find wrong data for key:%d %p -> %p\n",
				key[0], &key[0], data);
#ifdef DEBUH
	printf("Found key %d with data %p: %p\n", key[0], data, &key[0]);
#endif
}

void insert_to_node() 
{
	int i, idx;
	int key[] = {22, 10, 33, 15, 45};
	struct btree_node *node = new_btree_node();
	struct btree_node *right;
	void *data;
	if (!node)
		return;

	for (i = 0; i < ARRAY_SIZE(key); i++) {
		get_idx(node, key[i], &idx);
#ifdef DEBUH
		printf("key: %d may at idx %d\n", key[i], idx);
#endif
		btree_node_insert(node, idx, NULL, NULL, key[i], NULL);
		dump_btree_node(node, 0);
	}

	right = split_node(node, &i, &data);
#ifdef DEBUH
	printf("dump split left node:\n");
#endif
	dump_btree_node(node, 0);
#ifdef DEBUH
	printf("dump split right node:\n");
#endif
	dump_btree_node(right, 0);
}

void __insert_key(struct btree *tree, int *key, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		btree_insert(tree, key[i], &key[i]);
	}
	dump_btree(tree);
}

void insert_key()
{
	void *data;
	struct btree tree = BTREE;
	int key[] = {22, 33, 10, 15, 16,
		     7, 8, 9, 199, 120, 800,
		     2000, 1200, 130,
		     50, 100, 101,
		     110, 168, 198,
		};
	__insert_key(&tree, key, ARRAY_SIZE(key));

	data = btree_lookup(&tree, key[0]);
	if (data)
		printf("Found key %d with data %p\n", key[0], data);
	else
		printf("not found\n");
	// update the key
	btree_insert(&tree, key[0], &tree);
	printf("The tree's pointer is %p\n", &tree);
	data = btree_lookup(&tree, key[0]);
	if (data)
		printf("Found key %d with data %p\n", key[0], data);
}

void iterate_btree()
{
	int i;
	struct btree tree = BTREE;
	BTREE_ITERATOR(biter, &tree);

	btree_first(&biter);
	if (!biter.node)
		printf("There is no first entry in tree\n");
	btree_last(&biter);
	if (!biter.node)
		printf("There is no last entry in tree\n");

	int key1[] = {618 };
	__insert_key(&tree, key1, ARRAY_SIZE(key1));

	BTREE_ITERATOR_INIT(biter);
	btree_first(&biter);
	if (biter.node)
		printf("The first entry in tree is %d\n", biter.node->key[biter.idx]);
	BTREE_ITERATOR_INIT(biter);
	btree_last(&biter);
	if (biter.node)
		printf("The last entry in tree is %d\n", biter.node->key[biter.idx]);

	int key[] = {22, 33, 10, 15, 16,
		     7, 8, 9, 199, 120, 800,
		     900, 1034, 1056,24, 29, 30,
		     44, 45, 49, 34, 55, 99, 189,
		     188,
		};
	__insert_key(&tree, key, ARRAY_SIZE(key));

	BTREE_ITERATOR_INIT(biter);
	btree_first(&biter);
	if (biter.node)
		printf("The first entry in tree is %d\n", biter.node->key[biter.idx]);
	BTREE_ITERATOR_INIT(biter);
	btree_last(&biter);
	if (biter.node)
		printf("The last entry in tree is %d\n", biter.node->key[biter.idx]);

	BTREE_ITERATOR_INIT(biter);
	while (btree_next(&biter)) {
		// printf("node: %p, idx %d ", biter.node, biter.idx);
		printf("%5d", biter.node->key[biter.idx]);
	}
	printf("\n\n\n");

	BTREE_ITERATOR_INIT(biter);
	while (btree_prev(&biter)) {
		// printf("node: %p, idx %d ", biter.node, biter.idx);
		printf("%5d", biter.node->key[biter.idx]);
	}
	printf("\n");

	BTREE_ITERATOR_INIT(biter);
	for_each_btree(&biter)
		printf("%5d", biter.node->key[biter.idx]);
	printf("\n");
}

void delete_from_node()
{
	int i, idx;
	int key[] = {22, 10, 33, 15};
	struct btree_node *node = new_btree_node();
	struct btree_node *right;
	void *data;
	if (!node)
		return;

	for (i = 0; i < ARRAY_SIZE(key); i++) {
		get_idx(node, key[i], &idx);
		btree_node_insert(node, idx, NULL, NULL, key[i], NULL);
	}
#ifdef DEBUG
	printf("Now we have a node like: \n");
#endif
	dump_btree_node(node, 0);

	btree_node_delete(node, 2, false);
#ifdef DEBUG
	printf("After deletion idx 2 : \n");
#endif
	dump_btree_node(node, 0);

	btree_node_delete(node, 1, false);
#ifdef DEBUG
	printf("After deletion idx 1 : \n");
#endif
	dump_btree_node(node, 0);

	btree_node_replace(node, 0, key[0], &key[0]);
#ifdef DEBUG
	printf("After replace idx 0 with %d: \n", key[0]);
#endif
	dump_btree_node(node, 0);
}

void delete_key_test()
{
	void *data;
	int i;
	struct btree tree = BTREE;
	BTREE_ITERATOR(biter, &tree);
	int key[] = {22, 33, 10, 15, 16,
		     7, 199, 120, 800,
		     2000, 1200, 130,
		     50, 100, 101,
		     110, 168, 198,
		};

	prefix_reset();
	prefix_push("delete_key_test");
	test_print("Running %s tests...\n", __func__);

	PREFIX_PUSH();

	__insert_key(&tree, key, ARRAY_SIZE(key));

	data = btree_delete(&tree, -1);
	ASSERT_EQ(NULL, data);

	// delete first which is a leaf
	data = btree_delete(&tree, 7);
	ASSERT_NE(NULL, data);
#ifdef DEBUD
	printf("After delete key 7: \n");
	dump_btree(&tree);
#endif

	// delete one in root node
	data = btree_delete(&tree, tree.root->key[0]);
	ASSERT_NE(NULL, data);
#ifdef DEBUD
	printf("After delete key %d: \n", tree.root->key[0]);
	dump_btree(&tree);
#endif

	data = btree_delete(&tree, 15);
	ASSERT_NE(NULL, data);
#ifdef DEBUD
	printf("After delete key 15: \n");
	dump_btree(&tree);
#endif

	data = btree_delete(&tree, 16);
	ASSERT_NE(NULL, data);
#ifdef DEBUD
	printf("After delete key 16: \n");
	dump_btree(&tree);
#endif

	data = btree_delete(&tree, 10);
	ASSERT_NE(NULL, data);
#ifdef DEBUD
	printf("After delete key 10: \n");
	dump_btree(&tree);
#endif

	for (i = 0; i < ARRAY_SIZE(key); i++) {
		data = btree_delete(&tree, key[i]);

		if (data && data != &key[i])
			panic("delete a wrong data at %d\n", key[i]);
	}
	if (tree.root)
		panic("still has root %p\n", tree.root);

	test_pass_pop();

	prefix_pop();
}

int main(int argc, char *argv[])
{
	parse_args(argc, argv);

	// getidx_in_node();
	// lookup_key();
	// insert_key();
	// insert_to_node();
	// iterate_btree();
	// delete_from_node();

	delete_key_test();

}
