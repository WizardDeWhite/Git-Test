#include "xarray.h"
#include "bug.h"
#include "bitmap.h"

/*
 * Coding conventions in this file:
 *
 * @xa is used to refer to the entire xarray.
 * @xas is the 'xarray operation state'.  It may be either a pointer to
 * an xa_state, or an xa_state stored on the stack.  This is an unfortunate
 * ambiguity.
 * @index is the index of the entry being operated on
 * @mark is an xa_mark_t; a small number indicating one of the mark bits.
 * @node refers to an xa_node; usually the primary one being operated on by
 * this function.
 * @offset is the index into the slots array inside an xa_node.
 * @parent refers to the @xa_node closer to the head than @node.
 * @entry refers to something stored in a slot in the xarray
 */

static inline unsigned int xa_lock_type(const struct xarray *xa)
{
	return (__force unsigned int)xa->xa_flags & 3;
}

static inline void xas_lock_type(struct xa_state *xas, unsigned int lock_type)
{
	if (lock_type == XA_LOCK_IRQ)
		xas_lock_irq(xas);
	else if (lock_type == XA_LOCK_BH)
		xas_lock_bh(xas);
	else
		xas_lock(xas);
}

static inline void xas_unlock_type(struct xa_state *xas, unsigned int lock_type)
{
	if (lock_type == XA_LOCK_IRQ)
		xas_unlock_irq(xas);
	else if (lock_type == XA_LOCK_BH)
		xas_unlock_bh(xas);
	else
		xas_unlock(xas);
}

static inline bool xa_track_free(const struct xarray *xa)
{
	return xa->xa_flags & XA_FLAGS_TRACK_FREE;
}

static inline bool xa_zero_busy(const struct xarray *xa)
{
	return xa->xa_flags & XA_FLAGS_ZERO_BUSY;
}

static inline void xa_mark_set(struct xarray *xa, xa_mark_t mark)
{
	if (!(xa->xa_flags & XA_FLAGS_MARK(mark)))
		xa->xa_flags |= XA_FLAGS_MARK(mark);
}

static inline void xa_mark_clear(struct xarray *xa, xa_mark_t mark)
{
	if (xa->xa_flags & XA_FLAGS_MARK(mark))
		xa->xa_flags &= ~(XA_FLAGS_MARK(mark));
}

static inline unsigned long *node_marks(struct xa_node *node, xa_mark_t mark)
{
	return node->marks[(__force unsigned)mark];
}

static inline bool node_get_mark(struct xa_node *node,
		unsigned int offset, xa_mark_t mark)
{
	return test_bit(offset, node_marks(node, mark));
}

/* returns true if the bit was set */
static inline bool node_set_mark(struct xa_node *node, unsigned int offset,
				xa_mark_t mark)
{
	return __test_and_set_bit(offset, node_marks(node, mark));
}

/* returns true if the bit was set */
static inline bool node_clear_mark(struct xa_node *node, unsigned int offset,
				xa_mark_t mark)
{
	return __test_and_clear_bit(offset, node_marks(node, mark));
}

static inline bool node_any_mark(struct xa_node *node, xa_mark_t mark)
{
	return !bitmap_empty(node_marks(node, mark), XA_CHUNK_SIZE);
}

static inline void node_mark_all(struct xa_node *node, xa_mark_t mark)
{
	bitmap_fill(node_marks(node, mark), XA_CHUNK_SIZE);
}

#define mark_inc(mark) do { \
	mark = (__force xa_mark_t)((__force unsigned)(mark) + 1); \
} while (0)

/*
 * xas_squash_marks() - Merge all marks to the first entry
 * @xas: Array operation state.
 *
 * Set a mark on the first entry if any entry has it set.  Clear marks on
 * all sibling entries.
 */
static void xas_squash_marks(const struct xa_state *xas)
{
	unsigned int mark = 0;
	unsigned int limit = xas->xa_offset + xas->xa_sibs + 1;

	if (!xas->xa_sibs)
		return;

	do {
		unsigned long *marks = xas->xa_node->marks[mark];
		if (find_next_bit(marks, limit, xas->xa_offset + 1) == limit)
			continue;
		__set_bit(xas->xa_offset, marks);
		bitmap_clear(marks, xas->xa_offset + 1, xas->xa_sibs);
	} while (mark++ != (__force unsigned)XA_MARK_MAX);
}

/* extracts the offset within this node from the index */
static unsigned int get_offset(unsigned long index, struct xa_node *node)
{
	return (index >> node->shift) & XA_CHUNK_MASK;
}

static void xas_set_offset(struct xa_state *xas)
{
	xas->xa_offset = get_offset(xas->xa_index, xas->xa_node);
}

/* move the index either forwards (find) or backwards (sibling slot) */
static void xas_move_index(struct xa_state *xas, unsigned long offset)
{
	unsigned int shift = xas->xa_node->shift;
	xas->xa_index &= ~XA_CHUNK_MASK << shift;
	xas->xa_index += offset << shift;
}

static void xas_next_offset(struct xa_state *xas)
{
	xas->xa_offset++;
	xas_move_index(xas, xas->xa_offset);
}

static void *set_bounds(struct xa_state *xas)
{
	xas->xa_node = XAS_BOUNDS;
	return NULL;
}

/*
 * Starts a walk.  If the @xas is already valid, we assume that it's on
 * the right path and just return where we've got to.  If we're in an
 * error state, return NULL.  If the index is outside the current scope
 * of the xarray, return NULL without changing @xas->xa_node.  Otherwise
 * set @xas->xa_node to NULL and return the current head of the array.
 */
static void *xas_start(struct xa_state *xas)
{
	void *entry;

	if (xas_valid(xas))
		return xas_reload(xas);
	if (xas_error(xas))
		return NULL;

	entry = xa_head(xas->xa);
	if (!xa_is_node(entry)) {
		if (xas->xa_index)
			return set_bounds(xas);
	} else {
		if ((xas->xa_index >> xa_to_node(entry)->shift) > XA_CHUNK_MASK)
			return set_bounds(xas);
	}

	xas->xa_node = NULL;
	return entry;
}

static void *xas_descend(struct xa_state *xas, struct xa_node *node)
{
	unsigned int offset = get_offset(xas->xa_index, node);
	void *entry = xa_entry(xas->xa, node, offset);

	xas->xa_node = node;
	if (xa_is_sibling(entry)) {
		offset = xa_to_sibling(entry);
		entry = xa_entry(xas->xa, node, offset);
		if (node->shift && xa_is_node(entry))
			entry = XA_RETRY_ENTRY;
	}

	xas->xa_offset = offset;
	return entry;
}
/**
 * xas_load() - Load an entry from the XArray (advanced).
 * @xas: XArray operation state.
 *
 * Usually walks the @xas to the appropriate state to load the entry
 * stored at xa_index.  However, it will do nothing and return %NULL if
 * @xas is in an error state.  xas_load() will never expand the tree.
 *
 * If the xa_state is set up to operate on a multi-index entry, xas_load()
 * may return %NULL or an internal entry, even if there are entries
 * present within the range specified by @xas.
 *
 * Context: Any context.  The caller should hold the xa_lock or the RCU lock.
 * Return: Usually an entry in the XArray, but see description for exceptions.
 */
void *xas_load(struct xa_state *xas)
{
	void *entry = xas_start(xas);

	while (xa_is_node(entry)) {
		struct xa_node *node = xa_to_node(entry);

		if (xas->xa_shift > node->shift)
			break;
		entry = xas_descend(xas, node);
		if (node->shift == 0)
			break;
	}
	return entry;
}

#define XA_RCU_FREE	((struct xarray *)1)

#define radix_tree_node		xa_node
void radix_tree_node_rcu_free(struct rcu_head *head)
{
	struct radix_tree_node *node =
			container_of(head, struct radix_tree_node, rcu_head);

	/*
	 * Must only free zeroed nodes into the slab.  We can be left with
	 * non-NULL entries by radix_tree_free_nodes, so clear the entries
	 * and tags here.
	 */
	memset(node->slots, 0, sizeof(node->slots));
	memset(node->tags, 0, sizeof(node->tags));
	INIT_LIST_HEAD(&node->private_list);

	// kmem_cache_free(radix_tree_node_cachep, node);
	free(node);
}

static void xa_node_free(struct xa_node *node)
{
	XA_NODE_BUG_ON(node, !list_empty(&node->private_list));
	node->array = XA_RCU_FREE;
	// call_rcu(&node->rcu_head, radix_tree_node_rcu_free);
	radix_tree_node_rcu_free(&node->rcu_head);
}

/*
 * xas_destroy() - Free any resources allocated during the XArray operation.
 * @xas: XArray operation state.
 *
 * This function is now internal-only.
 */
static void xas_destroy(struct xa_state *xas)
{
	struct xa_node *next, *node = xas->xa_alloc;

	while (node) {
		XA_NODE_BUG_ON(node, !list_empty(&node->private_list));
		next = /* rcu_dereference_raw */(node->parent);
		radix_tree_node_rcu_free(&node->rcu_head);
		xas->xa_alloc = node = next;
	}
}

/**
 * xas_nomem() - Allocate memory if needed.
 * @xas: XArray operation state.
 * @gfp: Memory allocation flags.
 *
 * If we need to add new nodes to the XArray, we try to allocate memory
 * with GFP_NOWAIT while holding the lock, which will usually succeed.
 * If it fails, @xas is flagged as needing memory to continue.  The caller
 * should drop the lock and call xas_nomem().  If xas_nomem() succeeds,
 * the caller should retry the operation.
 *
 * Forward progress is guaranteed as one node is allocated here and
 * stored in the xa_state where it will be found by xas_alloc().  More
 * nodes will likely be found in the slab allocator, but we do not tie
 * them up here.
 *
 * Return: true if memory was needed, and was successfully allocated.
 */
bool xas_nomem(struct xa_state *xas, gfp_t gfp)
{
	if (xas->xa_node != XA_ERROR(-ENOMEM)) {
		xas_destroy(xas);
		return false;
	}
	// if (xas->xa->xa_flags & XA_FLAGS_ACCOUNT)
	// 	gfp |= __GFP_ACCOUNT;
	// xas->xa_alloc = kmem_cache_alloc_lru(radix_tree_node_cachep, xas->xa_lru, gfp);
	xas->xa_alloc = malloc(sizeof(struct xa_node));
	if (!xas->xa_alloc)
		return false;
	xas->xa_alloc->parent = NULL;
	XA_NODE_BUG_ON(xas->xa_alloc, !list_empty(&xas->xa_alloc->private_list));
	xas->xa_node = XAS_RESTART;
	return true;
}

/*
 * __xas_nomem() - Drop locks and allocate memory if needed.
 * @xas: XArray operation state.
 * @gfp: Memory allocation flags.
 *
 * Internal variant of xas_nomem().
 *
 * Return: true if memory was needed, and was successfully allocated.
 */
static bool __xas_nomem(struct xa_state *xas, gfp_t gfp)
	// __must_hold(xas->xa->xa_lock)
{
	unsigned int lock_type = xa_lock_type(xas->xa);

	if (xas->xa_node != XA_ERROR(-ENOMEM)) {
		xas_destroy(xas);
		return false;
	}
	// if (xas->xa->xa_flags & XA_FLAGS_ACCOUNT)
	// 	gfp |= __GFP_ACCOUNT;
	// if (gfpflags_allow_blocking(gfp)) {
	// 	xas_unlock_type(xas, lock_type);
	// 	xas->xa_alloc = kmem_cache_alloc_lru(radix_tree_node_cachep, xas->xa_lru, gfp);
	// 	xas_lock_type(xas, lock_type);
	// } else {
	//	xas->xa_alloc = kmem_cache_alloc_lru(radix_tree_node_cachep, xas->xa_lru, gfp);
	// }
	xas->xa_alloc = malloc(sizeof(struct xa_node));
	if (!xas->xa_alloc)
		return false;
	xas->xa_alloc->parent = NULL;
	XA_NODE_BUG_ON(xas->xa_alloc, !list_empty(&xas->xa_alloc->private_list));
	xas->xa_node = XAS_RESTART;
	return true;
}

static void xas_update(struct xa_state *xas, struct xa_node *node)
{
	if (xas->xa_update)
		xas->xa_update(node);
	else
		XA_NODE_BUG_ON(node, !list_empty(&node->private_list));
}

static void *xas_alloc(struct xa_state *xas, unsigned int shift)
{
	struct xa_node *parent = xas->xa_node;
	struct xa_node *node = xas->xa_alloc;

	if (xas_invalid(xas))
		return NULL;

	if (node) {
		xas->xa_alloc = NULL;
	} else {
		// gfp_t gfp = GFP_NOWAIT | __GFP_NOWARN;

		// if (xas->xa->xa_flags & XA_FLAGS_ACCOUNT)
		// 	gfp |= __GFP_ACCOUNT;

		// node = kmem_cache_alloc_lru(radix_tree_node_cachep, xas->xa_lru, gfp);
		node = malloc(sizeof(struct xa_node));
		if (!node) {
			xas_set_err(xas, -ENOMEM);
			return NULL;
		}
	}

	if (parent) {
		node->offset = xas->xa_offset;
		parent->count++;
		XA_NODE_BUG_ON(node, parent->count > XA_CHUNK_SIZE);
		xas_update(xas, parent);
	}
	XA_NODE_BUG_ON(node, shift > BITS_PER_LONG);
	XA_NODE_BUG_ON(node, !list_empty(&node->private_list));
	node->shift = shift;
	node->count = 0;
	node->nr_values = 0;
	// RCU_INIT_POINTER(node->parent, xas->xa_node);
	node->parent = xas->xa_node;
	node->array = xas->xa;

	return node;
}

#ifdef CONFIG_XARRAY_MULTI
/* Returns the number of indices covered by a given xa_state */
unsigned long xas_size(const struct xa_state *xas)
{
	return (xas->xa_sibs + 1UL) << xas->xa_shift;
}
#endif

/*
 * Use this to calculate the maximum index that will need to be created
 * in order to add the entry described by @xas.  Because we cannot store a
 * multi-index entry at index 0, the calculation is a little more complex
 * than you might expect.
 */
unsigned long xas_max(struct xa_state *xas)
{
	unsigned long max = xas->xa_index;

#ifdef CONFIG_XARRAY_MULTI
	if (xas->xa_shift || xas->xa_sibs) {
		unsigned long mask = xas_size(xas) - 1;
		max |= mask;
		if (mask == max) {
			max++;
		}
	}
#endif

	return max;
}

/* The maximum index that can be contained in the array without expanding it */
static unsigned long max_index(void *entry)
{
	if (!xa_is_node(entry))
		return 0;
	return (XA_CHUNK_SIZE << xa_to_node(entry)->shift) - 1;
}

static void xas_shrink(struct xa_state *xas)
{
	struct xarray *xa = xas->xa;
	struct xa_node *node = xas->xa_node;

	for (;;) {
		void *entry;

		XA_NODE_BUG_ON(node, node->count > XA_CHUNK_SIZE);
		if (node->count != 1)
			break;
		entry = xa_entry_locked(xa, node, 0);
		if (!entry)
			break;
		if (!xa_is_node(entry) && node->shift)
			break;
		if (xa_is_zero(entry) && xa_zero_busy(xa))
			entry = NULL;
		xas->xa_node = XAS_BOUNDS;

		// RCU_INIT_POINTER(xa->xa_head, entry);
		xa->xa_head = entry;
		if (xa_track_free(xa) && !node_get_mark(node, 0, XA_FREE_MARK))
			xa_mark_clear(xa, XA_FREE_MARK);

		node->count = 0;
		node->nr_values = 0;
		if (!xa_is_node(entry))
			// RCU_INIT_POINTER(node->slots[0], XA_RETRY_ENTRY);
			node->slots[0] = XA_RETRY_ENTRY;
		xas_update(xas, node);
		xa_node_free(node);
		if (!xa_is_node(entry))
			break;
		node = xa_to_node(entry);
		node->parent = NULL;
	}
}

/*
 * xas_delete_node() - Attempt to delete an xa_node
 * @xas: Array operation state.
 *
 * Attempts to delete the @xas->xa_node.  This will fail if xa->node has
 * a non-zero reference count.
 */
static void xas_delete_node(struct xa_state *xas)
{
	struct xa_node *node = xas->xa_node;

	for (;;) {
		struct xa_node *parent;

		XA_NODE_BUG_ON(node, node->count > XA_CHUNK_SIZE);
		if (node->count)
			break;

		parent = xa_parent_locked(xas->xa, node);
		xas->xa_node = parent;
		xas->xa_offset = node->offset;
		xa_node_free(node);

		if (!parent) {
			xas->xa->xa_head = NULL;
			xas->xa_node = XAS_BOUNDS;
			return;
		}

		parent->slots[xas->xa_offset] = NULL;
		parent->count--;
		XA_NODE_BUG_ON(parent, parent->count > XA_CHUNK_SIZE);
		node = parent;
		xas_update(xas, node);
	}

	if (!node->parent)
		xas_shrink(xas);
}

/**
 * xas_free_nodes() - Free this node and all nodes that it references
 * @xas: Array operation state.
 * @top: Node to free
 *
 * This node has been removed from the tree.  We must now free it and all
 * of its subnodes.  There may be RCU walkers with references into the tree,
 * so we must replace all entries with retry markers.
 */
static void xas_free_nodes(struct xa_state *xas, struct xa_node *top)
{
	unsigned int offset = 0;
	struct xa_node *node = top;

	for (;;) {
		void *entry = xa_entry_locked(xas->xa, node, offset);

		if (node->shift && xa_is_node(entry)) {
			node = xa_to_node(entry);
			offset = 0;
			continue;
		}
		if (entry)
			// RCU_INIT_POINTER(node->slots[offset], XA_RETRY_ENTRY);
			node->slots[offset] = XA_RETRY_ENTRY;
		offset++;
		while (offset == XA_CHUNK_SIZE) {
			struct xa_node *parent;

			parent = xa_parent_locked(xas->xa, node);
			offset = node->offset + 1;
			node->count = 0;
			node->nr_values = 0;
			xas_update(xas, node);
			xa_node_free(node);
			if (node == top)
				return;
			node = parent;
		}
	}
}

/*
 * xas_expand adds nodes to the head of the tree until it has reached
 * sufficient height to be able to contain @xas->xa_index
 */
static int xas_expand(struct xa_state *xas, void *head)
{
	struct xarray *xa = xas->xa;
	struct xa_node *node = NULL;
	unsigned int shift = 0;
	unsigned long max = xas_max(xas);

	if (!head) {
		if (max == 0)
			return 0;
		while ((max >> shift) >= XA_CHUNK_SIZE)
			shift += XA_CHUNK_SHIFT;
		return shift + XA_CHUNK_SHIFT;
	} else if (xa_is_node(head)) {
		node = xa_to_node(head);
		shift = node->shift + XA_CHUNK_SHIFT;
	}
	xas->xa_node = NULL;

	while (max > max_index(head)) {
		xa_mark_t mark = 0;

		XA_NODE_BUG_ON(node, shift > BITS_PER_LONG);
		node = xas_alloc(xas, shift);
		if (!node)
			return -ENOMEM;

		node->count = 1;
		if (xa_is_value(head))
			node->nr_values = 1;
		// RCU_INIT_POINTER(node->slots[0], head);
		node->slots[0] = head;

		/* Propagate the aggregated mark info to the new child */
		for (;;) {
			if (xa_track_free(xa) && mark == XA_FREE_MARK) {
				node_mark_all(node, XA_FREE_MARK);
				if (!xa_marked(xa, XA_FREE_MARK)) {
					node_clear_mark(node, 0, XA_FREE_MARK);
					xa_mark_set(xa, XA_FREE_MARK);
				}
			} else if (xa_marked(xa, mark)) {
				node_set_mark(node, 0, mark);
			}
			if (mark == XA_MARK_MAX)
				break;
			mark_inc(mark);
		}

		/*
		 * Now that the new node is fully initialised, we can add
		 * it to the tree
		 */
		if (xa_is_node(head)) {
			xa_to_node(head)->offset = 0;
			// rcu_assign_pointer(xa_to_node(head)->parent, node);
			xa_to_node(head)->parent = node;
		}
		head = xa_mk_node(node);
		xa->xa_head = head;
		xas_update(xas, node);

		shift += XA_CHUNK_SHIFT;
	}

	xas->xa_node = node;
	return shift;
}

/*
 * xas_create() - Create a slot to store an entry in.
 * @xas: XArray operation state.
 * @allow_root: %true if we can store the entry in the root directly
 *
 * Most users will not need to call this function directly, as it is called
 * by xas_store().  It is useful for doing conditional store operations
 * (see the xa_cmpxchg() implementation for an example).
 *
 * Return: If the slot already existed, returns the contents of this slot.
 * If the slot was newly created, returns %NULL.  If it failed to create the
 * slot, returns %NULL and indicates the error in @xas.
 */
static void *xas_create(struct xa_state *xas, bool allow_root)
{
	struct xarray *xa = xas->xa;
	void *entry;
	void **slot;
	struct xa_node *node = xas->xa_node;
	int shift;
	unsigned int order = xas->xa_shift;

	if (xas_top(node)) {
		entry = xa_head_locked(xa);
		xas->xa_node = NULL;
		if (!entry && xa_zero_busy(xa))
			entry = XA_ZERO_ENTRY;
		shift = xas_expand(xas, entry);
		if (shift < 0)
			return NULL;
		if (!shift && !allow_root)
			shift = XA_CHUNK_SHIFT;
		entry = xa_head_locked(xa);
		slot = &xa->xa_head;
	} else if (xas_error(xas)) {
		return NULL;
	} else if (node) {
		unsigned int offset = xas->xa_offset;

		shift = node->shift;
		entry = xa_entry_locked(xa, node, offset);
		slot = &node->slots[offset];
	} else {
		shift = 0;
		entry = xa_head_locked(xa);
		slot = &xa->xa_head;
	}

	while (shift > order) {
		shift -= XA_CHUNK_SHIFT;
		if (!entry) {
			node = xas_alloc(xas, shift);
			if (!node)
				break;
			if (xa_track_free(xa))
				node_mark_all(node, XA_FREE_MARK);
			// rcu_assign_pointer(*slot, xa_mk_node(node));
			*slot = xa_mk_node(node);
		} else if (xa_is_node(entry)) {
			node = xa_to_node(entry);
		} else {
			break;
		}
		entry = xas_descend(xas, node);
		slot = &node->slots[xas->xa_offset];
	}

	return entry;
}

/**
 * xas_create_range() - Ensure that stores to this range will succeed
 * @xas: XArray operation state.
 *
 * Creates all of the slots in the range covered by @xas.  Sets @xas to
 * create single-index entries and positions it at the beginning of the
 * range.  This is for the benefit of users which have not yet been
 * converted to use multi-index entries.
 */
void xas_create_range(struct xa_state *xas)
{
	unsigned long index = xas->xa_index;
	unsigned char shift = xas->xa_shift;
	unsigned char sibs = xas->xa_sibs;

	xas->xa_index |= ((sibs + 1UL) << shift) - 1;
	if (xas_is_node(xas) && xas->xa_node->shift == xas->xa_shift)
		xas->xa_offset |= sibs;
	xas->xa_shift = 0;
	xas->xa_sibs = 0;

	for (;;) {
		xas_create(xas, true);
		if (xas_error(xas))
			goto restore;
		if (xas->xa_index <= (index | XA_CHUNK_MASK))
			goto success;
		xas->xa_index -= XA_CHUNK_SIZE;

		for (;;) {
			struct xa_node *node = xas->xa_node;
			if (node->shift >= shift)
				break;
			xas->xa_node = xa_parent_locked(xas->xa, node);
			xas->xa_offset = node->offset - 1;
			if (node->offset != 0)
				break;
		}
	}

restore:
	xas->xa_shift = shift;
	xas->xa_sibs = sibs;
	xas->xa_index = index;
	return;
success:
	xas->xa_index = index;
	if (xas->xa_node)
		xas_set_offset(xas);
}

static void update_node(struct xa_state *xas, struct xa_node *node,
		int count, int values)
{
	if (!node || (!count && !values))
		return;

	node->count += count;
	node->nr_values += values;
	XA_NODE_BUG_ON(node, node->count > XA_CHUNK_SIZE);
	XA_NODE_BUG_ON(node, node->nr_values > XA_CHUNK_SIZE);
	xas_update(xas, node);
	if (count < 0)
		xas_delete_node(xas);
}

/**
 * xas_store() - Store this entry in the XArray.
 * @xas: XArray operation state.
 * @entry: New entry.
 *
 * If @xas is operating on a multi-index entry, the entry returned by this
 * function is essentially meaningless (it may be an internal entry or it
 * may be %NULL, even if there are non-NULL entries at some of the indices
 * covered by the range).  This is not a problem for any current users,
 * and can be changed if needed.
 *
 * Return: The old entry at this index.
 */
void *xas_store(struct xa_state *xas, void *entry)
{
	struct xa_node *node;
	void **slot = &xas->xa->xa_head;
	unsigned int offset, max;
	int count = 0;
	int values = 0;
	void *first, *next;
	bool value = xa_is_value(entry);

	if (entry) {
		bool allow_root = !xa_is_node(entry) && !xa_is_zero(entry);
		first = xas_create(xas, allow_root);
	} else {
		first = xas_load(xas);
	}

	if (xas_invalid(xas))
		return first;
	node = xas->xa_node;
	if (node && (xas->xa_shift < node->shift))
		xas->xa_sibs = 0;
	if ((first == entry) && !xas->xa_sibs)
		return first;

	next = first;
	offset = xas->xa_offset;
	max = xas->xa_offset + xas->xa_sibs;
	if (node) {
		slot = &node->slots[offset];
		if (xas->xa_sibs)
			xas_squash_marks(xas);
	}
	if (!entry)
		xas_init_marks(xas);

	for (;;) {
		/*
		 * Must clear the marks before setting the entry to NULL,
		 * otherwise xas_for_each_marked may find a NULL entry and
		 * stop early.  rcu_assign_pointer contains a release barrier
		 * so the mark clearing will appear to happen before the
		 * entry is set to NULL.
		 */
		// rcu_assign_pointer(*slot, entry);
		*slot = entry;
		if (xa_is_node(next) && (!node || node->shift))
			xas_free_nodes(xas, xa_to_node(next));
		if (!node)
			break;
		count += !next - !entry;
		values += !xa_is_value(first) - !value;
		if (entry) {
			if (offset == max)
				break;
			if (!xa_is_sibling(entry))
				entry = xa_mk_sibling(xas->xa_offset);
		} else {
			if (offset == XA_CHUNK_MASK)
				break;
		}
		next = xa_entry_locked(xas->xa, node, ++offset);
		if (!xa_is_sibling(next)) {
			if (!entry && (offset > max))
				break;
			first = next;
		}
		slot++;
	}

	update_node(xas, node, count, values);
	return first;
}

/**
 * xas_get_mark() - Returns the state of this mark.
 * @xas: XArray operation state.
 * @mark: Mark number.
 *
 * Return: true if the mark is set, false if the mark is clear or @xas
 * is in an error state.
 */
bool xas_get_mark(const struct xa_state *xas, xa_mark_t mark)
{
	if (xas_invalid(xas))
		return false;
	if (!xas->xa_node)
		return xa_marked(xas->xa, mark);
	return node_get_mark(xas->xa_node, xas->xa_offset, mark);
}

/**
 * xas_set_mark() - Sets the mark on this entry and its parents.
 * @xas: XArray operation state.
 * @mark: Mark number.
 *
 * Sets the specified mark on this entry, and walks up the tree setting it
 * on all the ancestor entries.  Does nothing if @xas has not been walked to
 * an entry, or is in an error state.
 */
void xas_set_mark(const struct xa_state *xas, xa_mark_t mark)
{
	struct xa_node *node = xas->xa_node;
	unsigned int offset = xas->xa_offset;

	if (xas_invalid(xas))
		return;

	while (node) {
		if (node_set_mark(node, offset, mark))
			return;
		offset = node->offset;
		node = xa_parent_locked(xas->xa, node);
	}

	if (!xa_marked(xas->xa, mark))
		xa_mark_set(xas->xa, mark);
}

/**
 * xas_clear_mark() - Clears the mark on this entry and its parents.
 * @xas: XArray operation state.
 * @mark: Mark number.
 *
 * Clears the specified mark on this entry, and walks back to the head
 * attempting to clear it on all the ancestor entries.  Does nothing if
 * @xas has not been walked to an entry, or is in an error state.
 */
void xas_clear_mark(const struct xa_state *xas, xa_mark_t mark)
{
	struct xa_node *node = xas->xa_node;
	unsigned int offset = xas->xa_offset;

	if (xas_invalid(xas))
		return;

	while (node) {
		if (!node_clear_mark(node, offset, mark))
			return;
		if (node_any_mark(node, mark))
			return;

		offset = node->offset;
		node = xa_parent_locked(xas->xa, node);
	}

	if (xa_marked(xas->xa, mark))
		xa_mark_clear(xas->xa, mark);
}

/**
 * xas_init_marks() - Initialise all marks for the entry
 * @xas: Array operations state.
 *
 * Initialise all marks for the entry specified by @xas.  If we're tracking
 * free entries with a mark, we need to set it on all entries.  All other
 * marks are cleared.
 *
 * This implementation is not as efficient as it could be; we may walk
 * up the tree multiple times.
 */
void xas_init_marks(const struct xa_state *xas)
{
	xa_mark_t mark = 0;

	for (;;) {
		if (xa_track_free(xas->xa) && mark == XA_FREE_MARK)
			xas_set_mark(xas, mark);
		else
			xas_clear_mark(xas, mark);
		if (mark == XA_MARK_MAX)
			break;
		mark_inc(mark);
	}
}

/**
 * xas_find() - Find the next present entry in the XArray.
 * @xas: XArray operation state.
 * @max: Highest index to return.
 *
 * If the @xas has not yet been walked to an entry, return the entry
 * which has an index >= xas.xa_index.  If it has been walked, the entry
 * currently being pointed at has been processed, and so we move to the
 * next entry.
 *
 * If no entry is found and the array is smaller than @max, the iterator
 * is set to the smallest index not yet in the array.  This allows @xas
 * to be immediately passed to xas_store().
 *
 * Return: The entry, if found, otherwise %NULL.
 */
void *xas_find(struct xa_state *xas, unsigned long max)
{
	void *entry;

	if (xas_error(xas) || xas->xa_node == XAS_BOUNDS)
		return NULL;
	if (xas->xa_index > max)
		return set_bounds(xas);

	if (!xas->xa_node) {
		xas->xa_index = 1;
		return set_bounds(xas);
	} else if (xas->xa_node == XAS_RESTART) {
		entry = xas_load(xas);
		if (entry || xas_not_node(xas->xa_node))
			return entry;
	} else if (!xas->xa_node->shift &&
		    xas->xa_offset != (xas->xa_index & XA_CHUNK_MASK)) {
		xas->xa_offset = ((xas->xa_index - 1) & XA_CHUNK_MASK) + 1;
	}

	xas_next_offset(xas);

	while (xas->xa_node && (xas->xa_index <= max)) {
		if (unlikely(xas->xa_offset == XA_CHUNK_SIZE)) {
			xas->xa_offset = xas->xa_node->offset + 1;
			xas->xa_node = xa_parent(xas->xa, xas->xa_node);
			continue;
		}

		entry = xa_entry(xas->xa, xas->xa_node, xas->xa_offset);
		if (xa_is_node(entry)) {
			xas->xa_node = xa_to_node(entry);
			xas->xa_offset = 0;
			continue;
		}
		if (entry && !xa_is_sibling(entry))
			return entry;

		xas_next_offset(xas);
	}

	if (!xas->xa_node)
		xas->xa_node = XAS_BOUNDS;
	return NULL;
}

static void *xas_result(struct xa_state *xas, void *curr)
{
	if (xa_is_zero(curr))
		return NULL;
	if (xas_error(xas))
		curr = xas->xa_node;
	return curr;
}

#if 0
/*
 * __xas_nomem() - Drop locks and allocate memory if needed.
 * @xas: XArray operation state.
 * @gfp: Memory allocation flags.
 *
 * Internal variant of xas_nomem().
 *
 * Return: true if memory was needed, and was successfully allocated.
 */
static bool __xas_nomem(struct xa_state *xas, gfp_t gfp)
	//__must_hold(xas->xa->xa_lock)
{
	// unsigned int lock_type = xa_lock_type(xas->xa);

	if (xas->xa_node != XA_ERROR(-ENOMEM)) {
		xas_destroy(xas);
		return false;
	}
	if (xas->xa->xa_flags & XA_FLAGS_ACCOUNT)
		gfp |= __GFP_ACCOUNT;
	/*
	if (gfpflags_allow_blocking(gfp)) {
		xas_unlock_type(xas, lock_type);
		xas->xa_alloc = kmem_cache_alloc_lru(radix_tree_node_cachep, xas->xa_lru, gfp);
		xas_lock_type(xas, lock_type);
	} else {
		xas->xa_alloc = kmem_cache_alloc_lru(radix_tree_node_cachep, xas->xa_lru, gfp);
	}
	*/
	xas->xa_alloc = malloc();
	if (!xas->xa_alloc)
		return false;
	xas->xa_alloc->parent = NULL;
	// XA_NODE_BUG_ON(xas->xa_alloc, !list_empty(&xas->xa_alloc->private_list));
	xas->xa_node = XAS_RESTART;
	return true;
}
#endif


/**
 * __xa_store() - Store this entry in the XArray.
 * @xa: XArray.
 * @index: Index into array.
 * @entry: New entry.
 * @gfp: Memory allocation flags.
 *
 * You must already be holding the xa_lock when calling this function.
 * It will drop the lock if needed to allocate memory, and then reacquire
 * it afterwards.
 *
 * Context: Any context.  Expects xa_lock to be held on entry.  May
 * release and reacquire xa_lock if @gfp flags permit.
 * Return: The old entry at this index or xa_err() if an error happened.
 */
void *__xa_store(struct xarray *xa, unsigned long index, void *entry, gfp_t gfp)
{
	XA_STATE(xas, xa, index);
	void *curr;

	if (WARN_ON_ONCE(xa_is_advanced(entry)))
		return XA_ERROR(-EINVAL);
	if (xa_track_free(xa) && !entry)
		entry = XA_ZERO_ENTRY;

	do {
		curr = xas_store(&xas, entry);
		if (xa_track_free(xa))
			xas_clear_mark(&xas, XA_FREE_MARK);
	} while (__xas_nomem(&xas, gfp));

	return xas_result(&xas, curr);
}

/**
 * xa_store() - Store this entry in the XArray.
 * @xa: XArray.
 * @index: Index into array.
 * @entry: New entry.
 * @gfp: Memory allocation flags.
 *
 * After this function returns, loads from this index will return @entry.
 * Storing into an existing multi-index entry updates the entry of every index.
 * The marks associated with @index are unaffected unless @entry is %NULL.
 *
 * Context: Any context.  Takes and releases the xa_lock.
 * May sleep if the @gfp flags permit.
 * Return: The old entry at this index on success, xa_err(-EINVAL) if @entry
 * cannot be stored in an XArray, or xa_err(-ENOMEM) if memory allocation
 * failed.
 */
void *xa_store(struct xarray *xa, unsigned long index, void *entry, gfp_t gfp)
{
	void *curr;

	xa_lock(xa);
	curr = __xa_store(xa, index, entry, gfp);
	xa_unlock(xa);

	return curr;
}

void xas_set_range(struct xa_state *xas, unsigned long first,
		unsigned long last)
{
	unsigned int shift = 0;
	unsigned long sibs = last - first;
	unsigned int offset = XA_CHUNK_MASK;

	xas_set(xas, first);

	while ((first & XA_CHUNK_MASK) == 0) {
		if (sibs < XA_CHUNK_MASK)
			break;
		if ((sibs == XA_CHUNK_MASK) && (offset < XA_CHUNK_MASK))
			break;
		shift += XA_CHUNK_SHIFT;
		if (offset == XA_CHUNK_MASK)
			offset = sibs & XA_CHUNK_MASK;
		sibs >>= XA_CHUNK_SHIFT;
		first >>= XA_CHUNK_SHIFT;
	}

	offset = first & XA_CHUNK_MASK;
	if (offset + sibs > XA_CHUNK_MASK)
		sibs = XA_CHUNK_MASK - offset;
	if ((((first + sibs + 1) << shift) - 1) > last)
		sibs -= 1;

	xas->xa_shift = shift;
	xas->xa_sibs = sibs;
}

/**
 * xa_store_range() - Store this entry at a range of indices in the XArray.
 * @xa: XArray.
 * @first: First index to affect.
 * @last: Last index to affect.
 * @entry: New entry.
 * @gfp: Memory allocation flags.
 *
 * After this function returns, loads from any index between @first and @last,
 * inclusive will return @entry.
 * Storing into an existing multi-index entry updates the entry of every index.
 * The marks associated with @index are unaffected unless @entry is %NULL.
 *
 * Context: Process context.  Takes and releases the xa_lock.  May sleep
 * if the @gfp flags permit.
 * Return: %NULL on success, xa_err(-EINVAL) if @entry cannot be stored in
 * an XArray, or xa_err(-ENOMEM) if memory allocation failed.
 */
void *xa_store_range(struct xarray *xa, unsigned long first,
		unsigned long last, void *entry, gfp_t gfp)
{
	XA_STATE(xas, xa, 0);

	if (WARN_ON_ONCE(xa_is_internal(entry)))
		return XA_ERROR(-EINVAL);
	if (last < first)
		return XA_ERROR(-EINVAL);

	do {
		xas_lock(&xas);
		if (entry) {
			unsigned int order = BITS_PER_LONG;
			if (last + 1)
				order = __ffs(last + 1);
			xas_set_order(&xas, last, order);
			xas_create(&xas, true);
			if (xas_error(&xas))
				goto unlock;
		}
		do {
			xas_set_range(&xas, first, last);
			xas_store(&xas, entry);
			if (xas_error(&xas))
				goto unlock;
			first += xas_size(&xas);
		} while (first <= last);
unlock:
		xas_unlock(&xas);
	} while (xas_nomem(&xas, gfp));

	return xas_result(&xas, NULL);
}
void xa_dump_node(const struct xa_node *node)
{
	unsigned i, j;

	if (!node)
		return;
	if ((unsigned long)node & 3) {
		printf("node %p\n", node);
		return;
	}

	printf("node %p %s %d parent %p shift %d count %d values %d "
		"array %p list %p %p marks",
		node, node->parent ? "offset" : "max", node->offset,
		node->parent, node->shift, node->count, node->nr_values,
		node->array, node->private_list.prev, node->private_list.next);
	for (i = 0; i < XA_MAX_MARKS; i++)
		for (j = 0; j < XA_MARK_LONGS; j++)
			printf(" %lx", node->marks[i][j]);
	printf("\n");
}

void xa_dump_index(unsigned long index, unsigned int shift, int level)
{
	if (!shift)
		printf("%lu: ", index);
	else if (shift >= BITS_PER_LONG)
		printf("0-%lu: ", ~0UL);
	else
		printf("%lu-%lu: ", index, index | ((1UL << shift) - 1));
}

void xa_dump_entry(const void *entry, unsigned long index,
		unsigned long shift, int level)
{
	if (!entry)
		return;

	printf("%*s", level * 4, " ");
	xa_dump_index(index, shift, level);

	if (xa_is_node(entry)) {
		if (shift == 0) {
			printf("%p\n", entry);
		} else {
			unsigned long i;
			struct xa_node *node = xa_to_node(entry);
			xa_dump_node(node);
			for (i = 0; i < XA_CHUNK_SIZE; i++)
				xa_dump_entry(node->slots[i],
				      index + (i << node->shift), node->shift,
				      level + 1);
		}
	} else if (xa_is_value(entry))
		printf("value %ld (0x%lx) [%p]\n", xa_to_value(entry),
						xa_to_value(entry), entry);
	else if (!xa_is_internal(entry))
		printf("%p\n", entry);
	else if (xa_is_retry(entry))
		printf("retry (%ld)\n", xa_to_internal(entry));
	else if (xa_is_sibling(entry))
		printf("sibling (slot %ld)\n", xa_to_sibling(entry));
	else if (xa_is_zero(entry))
		printf("zero (%ld)\n", xa_to_internal(entry));
	else
		printf("UNKNOWN ENTRY (%p)\n", entry);
}

void xa_dump(const struct xarray *xa)
{
	void *entry = xa->xa_head;
	unsigned int shift = 0;

	printf("xarray: %p head %p flags %x marks %d %d %d\n", xa, entry,
			xa->xa_flags, xa_marked(xa, XA_MARK_0),
			xa_marked(xa, XA_MARK_1), xa_marked(xa, XA_MARK_2));
	if (xa_is_node(entry))
		shift = xa_to_node(entry)->shift + XA_CHUNK_SHIFT;
	xa_dump_entry(entry, 0, shift, 0);
}
