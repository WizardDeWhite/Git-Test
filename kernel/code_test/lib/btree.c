#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "btree.h"

struct btree_node *new_btree_node()
{
	struct btree_node *node;

	node = malloc(sizeof(struct btree_node));
	if (!node)
		return NULL;
	memset(node, 0, sizeof(struct btree_node));
	return node;
}

/* 
 * Get index of key in this node.
 *
 * If contains key, index is set the current index.
 * If not contains key, index is set to the index expected to be inserted at.
 *
 * return true means exact match, false for not.
 */ 
bool idx_in_node(struct btree_node *node, int key, int *index)
{
	int i;
	bool found = false;

	for (i = 0; i < node->used; i++) {
		if (node->key[i] < key)
			continue;

		if (node->key[i] == key)
			found = true;
		break;
	}

	*index = i;
	return found;
}

/*
 * return true means key is found, false for not found
 *
 * if found, iter->node and iter->idx points to the one
 */
bool __btree_lookup(struct btree_iterator *iter, int key)
{
	struct btree *tree = iter->tree;
	struct btree_node *node = iter->node;

	if (iter->node == BTREE_START)
		iter->node = tree->root;

	while (iter->node) {
		if (idx_in_node(iter->node, key, &iter->idx)) {
			return true;
		}

		iter->node = iter->node->children[iter->idx];
	}
	return false;
}

void *btree_lookup(struct btree *tree, int key)
{
	BTREE_ITERATOR(biter, tree);
	return __btree_lookup(&biter, key) ?
		biter.node->data[biter.idx] : NULL;
}

void dump_btree_node(struct btree_node *node, int level)
{
	int idx;

	if (!node)
		return;

#ifdef DEBUG
	/* self:used -> parent(index_in_parent) */
	printf("%*s %p:%d -> %p(%d)\n", level * 4, " ",
			node, node->used,
			node->parent, node->parent_index);
#endif

	for (idx = 0; idx < node->used; idx++) {
		dump_btree_node(node->children[idx], level + 1);
		/* key at idx */
		printf("%*s %d\n", level * 4, " ", node->key[idx]);
	}
	dump_btree_node(node->children[idx], level + 1);
}

void dump_btree(struct btree *tree)
{
#ifdef DEBUG
	struct btree_node *node = tree->root;

	dump_btree_node(node, 0);
#endif
}

/*
 * Special case for insertion the first element of node.
 */
void __btree_node_insert_first(struct btree_node *node,
		struct btree_node *left, struct btree_node *right,
		int key, void *data)
{
	if (node->used != 0)
		panic("insert first element to non-empty node\n");

	node->key[0] = key;
	node->data[0] = data;
	node->used = 1;

	if (left) {
		node->children[0] = left;
		left->parent = node;
		left->parent_index = 0;
	}

	if (right) {
		node->children[1] = right;
		right->parent = node;
		right->parent_index = 1;
	}
}

/*
 *            idx
 *      +---+ +---+
 *      | a | | b |
 *      +---+ +---+
 *     |     |     |
 *     c0    c1    c2
 *
 *
 * We shift idx right, but leave the original left child there.
 *
 *            idx
 *      +---+ +---+ +---+
 *      | a | | n | | b |
 *      +---+ +---+ +---+
 *     |     |     |     |
 *     c0    c1    c'    c2
 *
 *
 */
// insert key at [idx]
// return false means everything is fine, true means needs split
bool btree_node_insert(struct btree_node *node, int idx,
		struct btree_node *right,
		int key, void *data)
{
	int i;
	if (node->used == ORDER)
		panic("node overflow\n");
	// shift right from idx
	for (i = node->used - 1; i >= idx; i--) {
		// shift key and data
		node->key[i+1] = node->key[i];
		node->data[i+1] = node->data[i];

		// shift right child
		node->children[i+2] = node->children[i+1];
		// shift children's parent_index too
		if (node->children[i+2])
			node->children[i+2]->parent_index++;
	}
	node->key[idx] = key;
	node->data[idx] = data;

	// based on current split_node operation, left child is not changed
	if (right) {
		node->children[idx+1] = right;
		right->parent = node;
		right->parent_index = idx+1;
	}
	node->used++;

	return (node->used == ORDER);
}

void btree_insert(struct btree *tree, int key, void *data)
{
	struct btree_node *node = tree->root;
	struct btree_node *left = NULL, *right = NULL;
	int idx;

	// empty tree, create node and set it to root
	if (!node) {
		node = new_btree_node();
		tree->root = node;
	}

	do {
		// key already exist, update data only
		if (idx_in_node(node, key, &idx)) {
			node->data[idx] = data;
			return;
		}
	} while (node->children[idx] && (node = node->children[idx]));

	//printf("not found key %d, try to insert to %p\n", key, node);
	while (btree_node_insert(node, idx, right, key, data)) {
		// node is full, let's split it
		right = split_node(node, &key, &data);
		left = node->parent ? NULL : node ;
		// insert the split key to parent
		node = node->parent;
		if (!node) {
			// we are the root,
			// allocate a new node and assign it to root
			node = new_btree_node();
			if (!node)
				panic("failed to allocate new node\n");
			tree->root = node;

			__btree_node_insert_first(node, left, right, key, data);
			return;
		}
		// get the index where to put key/data
		idx_in_node(node, key, &idx);
	}
}

/*
 * split node and insert extra element into parent
 *
 *                           idx
 *                     +---+ +---+
 *                     | A | | C |
 *                     +---+ +---+
 *                    |     |     |
 *                    c0    c1    c2
 *                          /
 *                      /
 *                  /
 *              /
 *       +---+ +---+ +---+ +---+ +---+
 *       | X | | Y | | B | | J | | K |
 *       +---+ +---+ +---+ +---+ +---+
 *      |     |     |     |     |     |
 *      c10   c11   c12   c20   c21   c22
 *
 *
 * After split
 *
 *                           idx
 *                     +---+ +---+ +---+
 *                     | A | | B | | C |
 *                     +---+ +---+ +---+
 *                    |     |     |     |
 *                    c0    c1    c'    c2
 *                          /     \
 *                      /             \
 *                  /                     \
 *              /                             \
 *       +---+ +---+                       +---+ +---+
 *       | X | | Y |                       | J | | K |
 *       +---+ +---+                       +---+ +---+
 *      |     |     |                     |     |     |
 *      c10   c11   c12                   c20   c21   c22
 *
 */
struct btree_node *split_node(struct btree_node *node, int *key, void **data)
{
	struct btree_node *right;
	int i, j;

	if (node->used != ORDER)
		panic("%s:%d try to split a non-full node\n", __FILE__, __LINE__);

	right = new_btree_node();
	if (!right)
		panic("%s:%d failed to allocate node on split\n", __FILE__, __LINE__);

	// assign middle one to key/data
	*key = node->key[PIVOT];
	*data = node->data[PIVOT];

	// assign right part to new node
	for (i = 0, j = PIVOT +1; j < ORDER; i++, j++) {
		right->key[i] = node->key[j];
		right->data[i] = node->data[j];
		right->children[i] = node->children[j];
		// adjust children's parent/parent_index in right
		if (right->children[i]) {
			right->children[i]->parent = right;
			right->children[i]->parent_index = i;
		}
	}
	// take the last child
	right->children[i] = node->children[j];
	if (right->children[i]) {
		right->children[i]->parent = right;
		right->children[i]->parent_index = i;
	}

	right->used = ORDER - PIVOT - 1;

	// clear right part in node
	node->used = PIVOT;
	return right;
}

struct btree_node *btree_first(struct btree_iterator *iter)
{
	struct btree *tree = iter->tree;
	struct btree_node *node = iter->node;

	if (node == BTREE_START)
		node = tree->root;

	// first entry in leftmost child
	while (node && node->children[0])
		node = node->children[0];

	iter->node = node;
	iter->idx = 0;
	return node;
}

struct btree_node *btree_last(struct btree_iterator *iter)
{
	struct btree *tree = iter->tree;
	struct btree_node *node = iter->node;

	if (node == BTREE_START)
		node = tree->root;

	// last entry in rightmost child
	while (node && node->children[node->used])
		node = node->children[node->used];

	iter->node = node;
	iter->idx = node? node->used - 1:0;
	return node;
}

struct btree_node *btree_next(struct btree_iterator *iter)
{
	struct btree *tree = iter->tree;
	struct btree_node *node = iter->node;
	struct btree_node *parent;

	if (node == BTREE_START) {
		btree_first(iter);
		goto out;
	}

	// leftmost child if has right child
	if (node->children[iter->idx+1]) {
		node = node->children[iter->idx+1];
		while (node->children[0])
			node = node->children[0];

		iter->node = node;
		iter->idx = 0;
		goto out;
	}

	// printf("iter index: %d\n", iter->idx);
	// or next sibling
	if (iter->idx + 1 < node->used) {
		iter->idx++;
		goto out;
	}

	// or the first ancestor who's child is left one
	while (node && node->parent) {
		parent = node->parent;

		//printf(" node(%p:%d) parent(%p:%d) index: %d\n",
		//	node, node->used, parent, parent->used, node->parent_index);
		if (node->parent_index < parent->used) {
			iter->node = parent;
			iter->idx = node->parent_index;
			goto out;
		}
		node = parent;
	}

	iter->node = NULL;
out:
	return iter->node;
}

struct btree_node *btree_prev(struct btree_iterator *iter)
{
	struct btree *tree = iter->tree;
	struct btree_node *node = iter->node;
	struct btree_node *parent;

	if (node == BTREE_START) {
		btree_last(iter);
		goto out;
	}

	// rightmost child if has left child
	if ((iter->idx >= 0) && node->children[iter->idx]) {
		node = node->children[iter->idx];
		while (node->children[node->used])
			node = node->children[node->used];

		iter->node = node;
		iter->idx = node->used - 1;
		goto out;
	}

	// or previous sibling
	if (iter->idx > 0) {
		iter->idx--;
		goto out;
	}

	// or the first ancestor who's child is right one
	while (node && node->parent) {
		parent = node->parent;

		//printf(" node(%p:%d) parent(%p:%d) index: %d\n",
		//	node, node->used, parent, parent->used, node->parent_index);
		if (node->parent_index > 0) {
			iter->node = parent;
			iter->idx = node->parent_index - 1;
			goto out;
		}
		node = parent;
	}

	iter->node = NULL;
out:
	return iter->node;
}

void *btree_node_delete(struct btree_node *node, int idx, bool keep)
{
	int i;
	void *data;

	if (idx >= node->used)
		panic("Deleting out of range\n");

	data = node->data[idx];

	// key shift left from idx + 1
	for (i = idx; i < node->used - 1; i++) {
		node->key[i] = node->key[i+1];
		node->data[i] = node->data[i+1];
	}

	// children shift left from idx + 1 or idx + 2
	i = keep ? idx + 1 : idx;
	for (; i < node->used; i++) {
		node->children[i] = node->children[i+1];
		if (node->children[i])
			node->children[i]->parent_index--;
	}

	node->used--;
	return data;
}

/* replace node[idx] with key/data */
void btree_node_replace(struct btree_node *node, int idx, int key, void *data)
{
	if (idx >= node->used)
		panic("%s:%d: replace idx out of range\n", __FILE__, __LINE__);

	node->key[idx] = key;
	node->data[idx] = data;
}

/*
 * There are four cases on deletion:
 * 1. In leaf node, just delete it
 * 2. Replace key/data with next, which must be in leaf. Then remove next and
 *    node point to next.
 * 3. If node->used < PIVOT
 *    a. if sibling->used >= PIVOT, rotate at parent
 *    b. else, merge with sibling
 */
void *btree_delete(struct btree *tree, int key)
{
	struct btree_node *node, *parent, *sibling = NULL;
	int index;
	void *data;
	BTREE_ITERATOR(biter, tree);

	if (!__btree_lookup(&biter, key))
		return NULL;

	node = biter.node;
	index = biter.idx;
	data = node->data[index];

	// case 1 & 2
	if (!is_leaf(node)) {
		// case 2
		btree_next(&biter);
		btree_node_replace(node, index,
			biter.node->key[biter.idx], biter.node->data[biter.idx]);
		node = biter.node;
		index = biter.idx;
	}

	// remove from leaf node
	btree_node_delete(node, index, false);

	while (is_low(node)) {
		parent = node->parent;
		if (!parent) {
			// if the root is empty, replace it
			if (is_empty(node)) {
				tree->root = node->children[0];
				if (node->children[0])
					node->children[0]->parent = NULL;
				free(node);
			}
			break;
		}

		if (node->parent_index+1 <= parent->used) { // !node->parent_index
			sibling = parent->children[node->parent_index+1];
		} else {
			sibling = parent->children[node->parent_index-1];
			node = sibling;
		}

		// case 3.a
		if (!is_low(sibling)) {
			btree_rotate(parent, node->parent_index);
			break;
		}

		// case 3.b
		btree_merge(parent, node->parent_index);
		node = parent;
	}

	return data;
}

/*
 * Rotate from right to left.
 *
 * append node[idx] to left child and move right child's first element to parent
 *
 *                           idx
 *                     +---+ +---+ +---+
 *                     | A | | B | | C |
 *                     +---+ +---+ +---+
 *                    |     |     |     |
 *                    c0    c1    c2    c3
 *                          /     \
 *                      /             \
 *                  /                     \
 *              /                             \
 *       +---+ +---+                       +---+ +---+ +---+ +---+
 *       | X | | Y |                       | J | | K | | L | | M |
 *       +---+ +---+                       +---+ +---+ +---+ +---+
 *      |     |     |                     |     |     |     |     |
 *      c10   c11   c12                   c20   c21   c22   c23   c24
 *
 *
 *
 * After rotate:
 *
 *                           idx
 *                     +---+ +---+ +---+
 *                     | A | | J | | C |
 *                     +---+ +---+ +---+
 *                    |     |     |     |
 *                    c0    c1    c2    c3
 *                          /     \
 *                      /             \
 *                  /                     \
 *              /                             \
 *       +---+ +---+ +---+                 +---+ +---+ +---+
 *       | X | | Y | | B |                 | K | | L | | M |
 *       +---+ +---+ +---+                 +---+ +---+ +---+
 *      |     |     |     |               |     |     |     |
 *      c10   c11   c12   c20             c21   c22   c23   c24
 *
 */
void btree_rotate(struct btree_node *node, int idx)
{
      struct btree_node *left, *right;

      left = node->children[idx];
      right = node->children[idx+1];

      // append node[idx] to left, also with right's first child
      btree_node_insert(left, left->used, right->children[0],
                      node->key[idx], node->data[idx]);

      // replace idx with right[0]
      btree_node_replace(node, idx, right->key[0], right->data[0]);

      // remove right[0]
      btree_node_delete(right, 0, false);
}

/*
 * merge node[idx] and it's left/right child to left
 *
 *                           idx
 *                     +---+ +---+ +---+
 *                     | A | | B | | C |
 *                     +---+ +---+ +---+
 *                    |     |     |     |
 *                    c0    c1    c2    c3
 *                          /     \
 *                      /             \
 *                  /                     \
 *              /                             \
 *       +---+                             +---+ +---+
 *       | X |                             | J | | K |
 *       +---+                             +---+ +---+
 *      |     |                           |     |     |
 *      c10   c11                         c20   c21   c22
 *
 *
 * After merge
 *
 *                           idx
 *                     +---+ +---+
 *                     | A | | C |
 *                     +---+ +---+
 *                    |     |     |
 *                    c0    c1    c3
 *                          /
 *                      /
 *                  /
 *              /
 *       +---+ +---+ +---+ +---+
 *       | X | | B | | J | | K |
 *       +---+ +---+ +---+ +---+
 *      |     |     |     |     |
 *      c10   c11   c20   c21   c22
 *
 */
void btree_merge(struct btree_node *node, int idx)
{
	struct btree_node *left, *right;
	int i;

	left = node->children[idx];
	right = node->children[idx+1];

	// append node[idx] to left
	btree_node_insert(left, left->used, right->children[0],
			node->key[idx], node->data[idx]);
	btree_node_delete(node, idx, true);

	// append right to left
	for (i = 0; i < right->used; i++) {
		btree_node_insert(left, left->used,
				right->children[i+1],
				right->key[i], right->data[i]);
	}

	// release right
	free(right);
}
