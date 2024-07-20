#include <stdio.h>
#include "btree.h"

void get_idx_test()
{
	int index;
	struct btree_node node;

	prefix_reset();
	prefix_push("get_idx_test");
	test_print("Running %s tests...\n", __func__);

	PREFIX_PUSH();

	node.key[0] = 22;
	node.key[1] = 33;
	node.used = 2;

	/* Find 22 at index 0 */
	ASSERT_TRUE(idx_in_node(&node, 22, &index));
	ASSERT_EQ(index, 0);

	/* No entry with key 32, expect to insert at 1*/
	ASSERT_FALSE(idx_in_node(&node, 23, &index));
	ASSERT_EQ(index, 1);
	/* No entry with key 4, expect to insert at 2 */
	ASSERT_FALSE(idx_in_node(&node, 43, &index));
	ASSERT_EQ(index, 2);

	node.key[2] = 42;
	node.key[3] = 53;
	node.used = 4;

	/* Find 53 at index 3 */
	ASSERT_TRUE(idx_in_node(&node, 53, &index));
	ASSERT_EQ(index, 3);
	/* No entry with key 50, expect to insert at 3 */
	ASSERT_FALSE(idx_in_node(&node, 50, &index));
	ASSERT_EQ(index, 3);
	/* No entry with key 1, expect to insert at 0 */
	ASSERT_FALSE(idx_in_node(&node, 1, &index));
	ASSERT_EQ(index, 0);

	test_pass_pop();

	prefix_pop();
}

void build_tree(struct btree *tree, int *key, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		btree_insert(tree, key[i], &key[i]);
	}
	dump_btree(tree);
}

void lookup_key()
{
	struct btree tree = BTREE;
	struct btree_node *node;
	int key[] = {22, 33, 10, 15};
	int index;
	void *data;

	prefix_reset();
	prefix_push("lookup_key");
	test_print("Running %s tests...\n", __func__);

	PREFIX_PUSH();

	data = btree_lookup(&tree, key[0]);
	/* Not expect to find entry in empty tree */
	ASSERT_EQ(NULL, data);

	build_tree(&tree, key, ARRAY_SIZE(key));

	/* Find entry for key[0] */
	data = btree_lookup(&tree, key[0]);
	ASSERT_NE(NULL, data);
	ASSERT_EQ(&key[0], data);

	/* Not expect to find non-exist key */
	data = btree_lookup(&tree, 9999);
	ASSERT_EQ(NULL, data);

	test_pass_pop();

	prefix_pop();
}

void insert_to_node() 
{
	int i, idx;
	/* This test case needs a full inserted node */
	int key[] = {22, 10, 33, 15, 45, 1};
	struct btree_node *node = new_btree_node();
	struct btree_node *right;
	void *data;

	prefix_reset();
	prefix_push("insert_to_node");
	test_print("Running %s tests...\n", __func__);

	PREFIX_PUSH();

	if (!node)
		return;

	for (i = 0; i < ARRAY_SIZE(key); i++) {
		idx_in_node(node, key[i], &idx);
		btree_node_insert(node, idx, NULL, NULL, key[i], NULL);
#ifdef DEBUG
		printf("key: %d may at idx %d\n", key[i], idx);
		dump_btree_node(node, 0);
#endif
	}

	right = split_node(node, &i, &data);
#ifdef DEBUG
	printf("dump split left node:\n");
	dump_btree_node(node, 0);
	printf("dump split right node:\n");
	dump_btree_node(right, 0);
#endif

	test_pass_pop();

	prefix_pop();
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

	prefix_reset();
	prefix_push("insert_key");
	test_print("Running %s tests...\n", __func__);

	PREFIX_PUSH();

	build_tree(&tree, key, ARRAY_SIZE(key));

	data = btree_lookup(&tree, key[0]);
	ASSERT_NE(NULL, data);
	ASSERT_EQ(&key[0], data);

	// update the key
	btree_insert(&tree, key[0], &tree);
#ifdef DEBUG
	printf("The tree's pointer is %p\n", &tree);
#endif
	data = btree_lookup(&tree, key[0]);
	ASSERT_NE(NULL, data);
	ASSERT_EQ(&tree, data);

	test_pass_pop();

	prefix_pop();
}

void iterate_btree()
{
	int i;
	struct btree tree = BTREE;
	BTREE_ITERATOR(biter, &tree);

	prefix_reset();
	prefix_push("iterate_btree");
	test_print("Running %s tests...\n", __func__);

	PREFIX_PUSH();

	/* Empty tree iteration result in NULL node */
	btree_first(&biter);
	ASSERT_EQ(NULL, biter.node);
	btree_last(&biter);
	ASSERT_EQ(NULL, biter.node);

	int key1[] = {618 };
	build_tree(&tree, key1, ARRAY_SIZE(key1));

	/* After insertion, the first entry points to it */
	BTREE_ITERATOR_INIT(biter);
	btree_first(&biter);
	ASSERT_NE(NULL, biter.node);
	ASSERT_EQ(biter.node->key[biter.idx], key1[0]);
	ASSERT_EQ(biter.node->data[biter.idx], &key1[0]);

	/* Since there is only one entry, the last entry points to it too */
	BTREE_ITERATOR_INIT(biter);
	btree_last(&biter);
	ASSERT_NE(NULL, biter.node);
	ASSERT_EQ(biter.node->key[biter.idx], key1[0]);
	ASSERT_EQ(biter.node->data[biter.idx], &key1[0]);

	int key[] = {22, 33, 10, 15, 16,
		     7, 8, 9, 199, 120, 800,
		     900, 1034, 1056,24, 29, 30,
		     44, 45, 49, 34, 55, 99, 189,
		     188,
		};
	int key_ordered[] = {7, 8, 9, 10, 15, 16, 22,
		24, 29, 30, 33, 34, 44, 45, 49, 55,
		99, 120, 188, 189, 199, 
		618, /* from key1[] */
		800, 900, 1034, 1056,
		};
	build_tree(&tree, key, ARRAY_SIZE(key));

	/* The new tree's smallest entry is 7 */
	BTREE_ITERATOR_INIT(biter);
	btree_first(&biter);
	ASSERT_NE(NULL, biter.node);
	ASSERT_EQ(biter.node->key[biter.idx], key_ordered[0]);

	/* The new tree's largest entry is 1056 */
	BTREE_ITERATOR_INIT(biter);
	btree_last(&biter);
	ASSERT_NE(NULL, biter.node);
	ASSERT_EQ(biter.node->key[biter.idx], key_ordered[ARRAY_SIZE(key_ordered) - 1]);

	BTREE_ITERATOR_INIT(biter);
	i = 0;
	while (btree_next(&biter)) {
		ASSERT_NE(NULL, biter.node);
		ASSERT_EQ(biter.node->key[biter.idx], key_ordered[i]);
		i++;
	}

	i--;
	BTREE_ITERATOR_INIT(biter);
	while (btree_prev(&biter)) {
		ASSERT_NE(NULL, biter.node);
		ASSERT_EQ(biter.node->key[biter.idx], key_ordered[i]);
		i--;
	}

	BTREE_ITERATOR_INIT(biter);
	i = 0;
	for_each_btree(&biter) {
		ASSERT_NE(NULL, biter.node);
		ASSERT_EQ(biter.node->key[biter.idx], key_ordered[i]);
		i++;
	}

	test_pass_pop();

	prefix_pop();
}

void delete_from_node()
{
	int i, idx;
	int key[] = {22, 10, 33, 15};
	int key_ordered[] = {10, 15, 22, 33};
	struct btree_node *node = new_btree_node();
	struct btree_node *right;
	void *data;

	prefix_reset();
	prefix_push("delete_from_node");
	test_print("Running %s tests...\n", __func__);

	PREFIX_PUSH();

	if (!node)
		return;

	for (i = 0; i < ARRAY_SIZE(key); i++) {
		idx_in_node(node, key[i], &idx);
		btree_node_insert(node, idx, NULL, NULL, key[i], &key[i]);
	}
#ifdef DEBUG
	printf("Now we have a node like: \n");
	dump_btree_node(node, 0);
#endif

	/* There are total ARRAY_SIZE(key) entries in node */
	ASSERT_EQ(ARRAY_SIZE(key), node->used);

	data = btree_node_delete(node, 2, false);
#ifdef DEBUG
	printf("After deletion idx 2 : \n");
	dump_btree_node(node, 0);
#endif
	ASSERT_EQ(ARRAY_SIZE(key) - 1, node->used);
	ASSERT_EQ(key_ordered[2], *(int*)data);

	data = btree_node_delete(node, 1, false);
#ifdef DEBUG
	printf("After deletion idx 1 : \n");
	dump_btree_node(node, 0);
#endif
	ASSERT_EQ(ARRAY_SIZE(key) - 2, node->used);
	ASSERT_EQ(key_ordered[1], *(int*)data);

	btree_node_replace(node, 0, key[0], &key[0]);
#ifdef DEBUG
	printf("After replace idx 0 with %d: \n", key[0]);
	dump_btree_node(node, 0);
#endif

	test_pass_pop();

	prefix_pop();
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

	build_tree(&tree, key, ARRAY_SIZE(key));

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

	get_idx_test();
	lookup_key();
	insert_key();
	insert_to_node();
	iterate_btree();
	delete_from_node();
	delete_key_test();

}
