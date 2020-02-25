#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#include "bst.h"

struct BST_Node {
	struct BST_Node* left;
	struct BST_Node* right;
	uint64_t data;
};
#define BST_NODE_SIZE sizeof(struct BST_Node)

static uint64_t BST_count = 0;

uint64_t getBSTCount()
{
	return BST_count;
}

struct BST_Node* create_node(uint64_t data)
{
	struct BST_Node *new_node = malloc(BST_NODE_SIZE);
	new_node->data = data;
	new_node->left = NULL;
	new_node->right = NULL;
	return new_node;
}


int32_t BST_insert(struct BST_Node** node, uint64_t data)
{
	while (*node) {
		if ((*node)->data == data)
			return 0;
		node = ((*node)->data > data) ? &(*node)->left : &(*node)->right;
	}
	*node = create_node(data);
	BST_count++;
	return 1;
}
