#include "rope.h"
#include "string.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct node_tag *Node;

struct node_tag
{
	bool is_leaf;
	size_t height;
	size_t len; /* without NUL */
	Node left, right;
	int ref_count;
	char str[];
};

static Node
node_ref(Node node)
{
	node->ref_count++;
	return node;
}

static void
node_deref(Node node)
{
	node->ref_count--;
	if (node->ref_count == 0)
		pfree(node);
}

static Node
CreateConcat(Node left, Node right)
{
	Node node = palloc(sizeof(*node));

	node->is_leaf = false;
	node->height = left->height > right->height ? left->height + 1 : right->height + 1;
	node->ref_count = 1;
	node->len = 0;
	if (left)
		node->len += left->len;
	if (right)
		node->len += right->len;
	node->left = left;
	node->right = right;
	node->str[0] = '\0'; /* XXX: for safety */

	return node;
}

static Node
CreateLeaf(char *str, size_t len)
{
	Node node = palloc(sizeof(*node) + len + 1);

	node->is_leaf = true;
	node->height = 0;
	node->ref_count = 1;
	node->len = len;
	node->left = node->right = NULL;
	memcpy(node->str, str, len);
	node->str[len] = '\0';

	return node;
}

static void
SubtreeDestructChain(Node root)
{
	if (root->left)
		SubtreeDestructChain(root->left);
	if (root->right)
		SubtreeDestructChain(root->right);
	node_deref(root);
}

static void
subtree_dump(Node node, int level)
{
	for (int i = 0; i < level; i++)
		printf(" ");
	printf("| ");

	if (node->is_leaf)
		printf("Leaf: len=%zu, str=%s, refcount=%d\n", node->len, node->str, node->ref_count);
	else
	{
		printf("Concat: height=%zu, len=%zu, refcount=%d\n", node->height, node->len, node->ref_count);
		subtree_dump(node->left, level+1);
		subtree_dump(node->right, level+1);
	}
}

static void
SubtreeDump(Node root)
{
	assert(root);
	subtree_dump(root, 0);
}

struct rope_tag
{
	Node root;
};

Rope
RopeCreate(char str[], size_t len)
{
	Rope rope = palloc(sizeof(*rope));

	assert(str);

	rope->root = CreateLeaf(str, len);

	return rope;
}

void
RopeDestroy(Rope rope)
{
	assert(rope);
	SubtreeDestructChain(rope->root);
	pfree(rope);
}

void
RopeDump(Rope rope)
{
	assert(rope);
	SubtreeDump(rope->root);
}

static int
rope_collect_cstr(Node node, char *ret_buf, int i)
{
	if (node->is_leaf)
	{
		printf("i=%d buf_ptr=%p, node->str=%s, node->len=%zu\n", i, ret_buf, node->str, node->len);
		memcpy(ret_buf + i, node->str, node->len);
		return node->len;
	}

	i += rope_collect_cstr(node->left, ret_buf, i);
	return rope_collect_cstr(node->right, ret_buf, i);
}

int
RopeToString(Rope rope, char *ret_buf, size_t buf_size)
{
	int rv;

	if (rope->root->len > buf_size - 1)
		return -1;

	rv = rope_collect_cstr(rope->root, ret_buf, 0);
	ret_buf[rope->root->len] = '\0';

	return rv;
}

size_t
RopeGetLen(Rope rope)
{
	assert(rope);
	return rope->root->len;
}

Rope
RopeConcat(Rope left, Rope right)
{
	assert(left);
	assert(right);

	{
		Rope new_rope = palloc(sizeof(*new_rope));
		new_rope->root = CreateConcat(node_ref(left->root), node_ref(right->root));

		return new_rope;
	}
}

static Node
node_get_substr(Node node, size_t i, size_t n)
{
	if (node->is_leaf)
	{
		if (n == node->len)
			return node_ref(node);

		return CreateLeaf(((char *)node->str) + i, n);
	}
	else
	{
		size_t llen = node->left->len;

		if (llen >= i + n)
			return node_get_substr(node->left, i, n);
		else if (llen <= i)
			return node_get_substr(node->right, i - llen, n);
		else
			return CreateConcat(
					node_get_substr(node->left, i, llen - i),
					node_get_substr(node->right, 0, n - llen + i));
	}
}

Rope
RopeSubstr(Rope rope, size_t i, size_t n)
{
	Rope new_rope = palloc(sizeof(*new_rope));

	assert(rope);
	assert(n != 0);
	assert(i + n <= rope->root->len);

	new_rope->root = node_get_substr(rope->root, i, n);

	return new_rope;
}

char
RopeIndex(Rope rope, size_t i)
{
	Node node = rope->root;

	assert(rope);
	assert(i < node->len);

	for (;;)
	{
		if (node->is_leaf)
			return node->str[i];
		else
		{
			size_t llen = node->left->len;

			if (i < llen)
				node = node->left;
			else
			{
				node = node->right;
				i -= llen;
			}
		}
	}
}

#define ROPE_SCAN_MAX_DEPTH 20

typedef enum
{
	LEFT_DOWN,
	RIGHT_DOWN,
} scan_dir;

struct rope_scan_tag
{
	size_t pos, depth;
	scan_dir dir[ROPE_SCAN_MAX_DEPTH];
	Node stack[ROPE_SCAN_MAX_DEPTH];
};

RopeScan
RopeScanInit(Rope rope)
{
	Node node = rope->root;
	RopeScan scan = palloc(sizeof(*scan));

	scan->pos = scan->depth = 0;

	while (!node->is_leaf)
	{
		scan->stack[scan->depth] = node;
		scan->dir[scan->depth] = LEFT_DOWN;
		scan->depth++;
		node = node->left;
	}

	return scan;
}

char
RopeScanGetNext(RopeScan scan)
{
	Node node = scan->stack[scan->depth];

	for (;;)
	{
		if (scan->pos < node->len)
			return node->str[scan->pos++];

		do
		{
			if (scan->depth == 0) /* End of scan */
				return 0;			/* FIXME: Is NUL appropriate? */
			scan->depth--;
		} while (scan->dir[scan->depth] == RIGHT_DOWN);

		scan->dir[scan->depth] = RIGHT_DOWN;
		node = scan->stack[scan->depth];
		scan->depth++;

		while (!node->is_leaf)
		{
			scan->stack[scan->depth] = node;
			scan->dir[scan->depth] = LEFT_DOWN;
			scan->depth++;
			node = node->left;
		}

		scan->pos = 0;
	}
}

void
RopeScanFini(RopeScan scan)
{
	pfree(scan);
}
