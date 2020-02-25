#pragma once

#include <stdint.h>

uint64_t getBSTCount();
struct BST_Node;
int BST_insert(struct BST_Node** root, uint64_t data);
