#pragma once

#include <cstddef>
#include <cstdint>
#include <memory> //smart pointer library -> better allocation of memory

struct AVLNode {
    uint32_t depth{0};
    uint32_t cnt{0};
    std::unique_ptr<AVLNode> left = nullptr;
    std::unique_ptr<AVLNode> right = nullptr;
    AVLNode* parent = nullptr;
};
using AVLNodePtr = std::unique_ptr<AVLNode>;

inline static void avl_init(AVLNode *node){
	node->depth=1;
	node->cnt=1;
	node->left=nullptr;
	node->right=nullptr;
	node->parent=nullptr;
}

static auto avl_fix(AVLNodePtr node) -> AVLNodePtr;
static auto avl_del(AVLNodePtr node) -> AVLNodePtr;
static auto avl_offset(const AVLNode* node, int64_t offset) -> std::unique_ptr<AVLNode>;
