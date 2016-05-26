#include "rope.h"
#include "string.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct node_tag *Node;

struct node_tag
{
	bool is_leaf;
	size_t depth;
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

	/* XXX: implement */
	node->depth = 0;

	node->ref_count = 1;

	node->len = 0;
	if (left)
		node->len += left->len;
	if (right)
		node->len += right->len;

	node->is_leaf = false;
	node->left = node_ref(left);
	node->right = node_ref(right);

	return node;
}

static Node
CreateLeaf(char *str, size_t len)
{
	Node node = palloc(sizeof(*node) + len + 1);

	node->str[len] = '\0';

	node->ref_count = 1;
	node->len = len;
	node->is_leaf = true;
	node->left = node->right = NULL;
	memcpy(node->str, str, len);

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
subtree_to_s(Node node, int level)
{
	for (int i = 0; i < level; i++)
		printf(" ");
	printf("| ");

	if (node->is_leaf)
		printf("Leaf: len=%zu, str=%s\n", node->len, node->str);
	else
	{
		printf("Concat: depth=%zu, len=%zu\n", node->depth, node->len);
		subtree_to_s(node->left, level+1);
		subtree_to_s(node->right, level+1);
	}
}

static void
SubtreeToString(Node root)
{
	subtree_to_s(root, 0);
}

struct rope_tag
{
	Node root;
};

Rope
RopeCreate(char str[], size_t len)
{
	Rope rope = palloc(sizeof(*rope));

	rope->root = CreateLeaf(str, len);

	return rope;
}

void
RopeDestroy(Rope rope)
{
	SubtreeDestructChain(rope->root);
	pfree(rope);
}

void
RopeToString(Rope rope)
{
	SubtreeToString(rope->root);
}

Rope
RopeConcat(Rope left, Rope right)
{
	if (!left)
		return right;
	if (!right)
		return left;

	{
		Rope new_rope = palloc(sizeof(*new_rope));
		new_rope->root = CreateConcat(left->root, right->root);

		return new_rope;
	}
}
