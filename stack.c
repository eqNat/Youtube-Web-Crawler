#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#include "stack.h"

struct Stack_Node {
	uint64_t data;
	struct Stact_Node* next;
};
#define STACK_NODE_SIZE sizeof(struct Stack_Node)

static uint64_t stack_count = 0;

uint64_t getStackCount()
{
	return stack_count;
}

void push(struct Stack_Node** node, uint64_t data)
{		
	struct Stack_Node* new_node = malloc(STACK_NODE_SIZE);
	new_node->data = data;
	new_node->next = *node;
	*node = new_node;
	stack_count++;
}

uint64_t pop(struct Stack_Node** node)
{
	uint64_t temp;
	if (*node == NULL) {
		fprintf(stderr, "Stack's empty\n");
		return 0;
	}
	struct Stack_Node* old_node = *node;
	temp = (*node)->data;
	*node = (*node)->next;
	free(old_node);
	stack_count--;
	return temp;
}

