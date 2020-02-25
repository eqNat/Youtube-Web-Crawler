#pragma once

#include <stdint.h>

struct Stack_Node;

void push(struct Stack_Node** node, uint64_t data);
uint64_t pop(struct Stack_Node** node);
