#include <cstddef>
#include <cstdint>
#include <memory> //smart pointer library -> better allocation of memory
#include <tuple>
#include <cassert>


/* 
Adelson-Velskii & Landis

AVL trees are balanced so that no subtree of any node is remains more than one node taller than the opposite subtree. The difference in height is the balance factor, kept in a tag in each node. During insertions or deltions, the balance factor may transiently be off-balance by +/- 2, but growth (positive or negative) is handled by rotating to mitigate this imbalance before returning from the function that operates on that node.
https://www2.hawaii.edu/~tvs/avl-tree/#:~:text=Tree%20Data%20Structures,a%20tag%20in%20each%20node.

*/


// the balance factor of each node is maintained to ensure logarithmic time complexity for insertions, deletions, and lookups.


namespace AVLTree{

	struct AVLNode {
	    uint32_t depth = 0;
	    uint32_t cnt = 0;
	    std::unique_ptr<AVLNode> left = nullptr;
	    std::unique_ptr<AVLNode> right = nullptr;
	    AVLNode* parent = nullptr;
	};

	using AVLNodePtr = std::unique_ptr<AVLNode>;

	static void avl_init(AVLNode* node) {
	    node->depth = 1;
	    node->cnt = 1;
	    node->left = nullptr;
	    node->right = nullptr;
	    node->parent = nullptr;
	}

	static uint32_t avl_depth(const AVLNode* node) const {
	    return node ? node->depth : 0;
	}

	static uint32_t avl_cnt(const AVLNode* node) const {
	    return node ? node->cnt : 0;
	}

	static uint32_t max(uint32_t lhs, uint32_t rhs) {
	    return lhs < rhs ? rhs : lhs;
	}

	static void avl_update(AVLNode* node) {
	    node->depth = 1 + max(avl_depth(node->left.get()), avl_depth(node->right.get()));
	    node->cnt = 1 + avl_cnt(node->left.get()) + avl_cnt(node->right.get());
	}

	static AVLNodePtr rot_left(AVLNodePtr node) {
	    AVLNodePtr new_node = std::move(node->right);
	    if (new_node->left) {
		new_node->left->parent = node.get();
	    }
	    node->right = std::move(new_node->left);
	    new_node->left = std::move(node);
	    new_node->parent = node->parent;
	    node->parent = new_node;

	    avl_update(node.get());
	    avl_update(new_node.get());
	    return new_node;
	}

	static AVLNodePtr rot_right(AVLNodePtr node) {
	    AVLNodePtr new_node = std::move(node->left);
	    if (new_node->right) {
		new_node->right->parent = node.get();
	    }
	    node->left = std::move(new_node->right);
	    new_node->right = std::move(node);
	    new_node->parent = node->parent;
	    node->parent = new_node;
	    avl_update(node.get());
	    avl_update(new_node.get());
	    return new_node;
	}

	// if left tree is too deep
	static AVLNodePtr avl_fix_left(AVLNodePtr root) {
	    auto rotate_left = [](AVLNodePtr node) {
		return rot_left(std::move(node));
	    };

	    if (avl_depth(root->left->left.get()) < avl_depth(root->left->right.get())) {
		root->left = rotate_left(std::move(root->left));
	    }
	    return rot_right(std::move(root));
	}

	// if right subtree is too deep
	static AVLNodePtr avl_fix_right(AVLNodePtr root) {
	    auto rotate_right = [](AVLNodePtr node) {
		return rot_right(std::move(node));
	    };

	    if (avl_depth(root->right->right.get()) < avl_depth(root->right->left.get())) {
		root->right = rotate_right(std::move(root->right));
	    }
	    return rotate_left(std::move(root));
	}

	// fix imbalanced nodes & mantain invariants until the roots is reached
	static AVLNodePtr avl_fix(AVLNodePtr node) {
	    while (true) {
		avl_update(node.get());
		uint32_t l = avl_depth(node->left.get());
		uint32_t r = avl_depth(node->right.get());
		AVLNode** from = nullptr;
		if (node->parent) {
		    from = (node->parent->left.get() == node) ? &node->parent->left : &node->parent->right;
		}
		if (l == r + 2) {
		    node = avl_fix_left(std::move(node));
		} else if (l + 2 == r) {
		    node = avl_fix_right(std::move(node));
		}

		if (!from) {
		    return std::move(node);
		}
		*from = std::move(node);
		node = (*from)->parent;
	    }
	}

	// Detach a node and returns the new root of the tree
	static AVLNodePtr avl_del(AVLNodePtr node) {
	    if (!node->right) {
		AVLNode* parent = node->parent;
		if (node->left) {
		    node->left->parent = parent;
		}
		if (parent) {
		    if (parent->left.get() == node) {
		        parent->left = std::move(node->left);
		    } else {
		        parent->right = std::move(node->left);
		    }
		    return avl_fix(std::move(parent));
		} else {
		    return std::move(node->left);
		}
	    } else {
		AVLNodePtr victim = node->right.get();
		while (victim->left) {
		    victim = victim->left.get();
		}
		AVLNodePtr root = avl_del(std::move(victim));

		*victim = *node;
		if (victim->left) {
		    victim->left->parent = victim.get();
		}
		if (victim->right) {
		    victim->right->parent = victim.get();
		}
		AVLNode* parent = node->parent;
		if (parent) {
		    if (parent->left.get() == node) {
		        parent->left = std::move(victim);
		    } else {
		        parent->right = std::move(victim);
		    }
		    return avl_fix(std::move(root));
		} else {
		    return std::move(victim);
		}
	    }
	}
	
	// offset into the succeeding or preceding node
	// node: worst-case complexity is O(log(n)) regardless of how long the offset is.
	std::unique_ptr<AVLNode> avl_offset(const AVLNode* node, int64_t offset) {
	    int64_t pos = 0;
	    while (offset != pos) {
		if (pos < offset && pos + avl_cnt(node->right.get()) >= offset) {
		    node = node->right.get();
		    pos += avl_cnt(node->left.get()) + 1;
		} else if (pos > offset && pos - avl_cnt(node->left.get()) <= offset) {
		    node = node->left.get();
		    pos -= avl_cnt(node->right.get()) + 1;
		} else {
		    AVLNode* parent = node->parent;
		    if (!parent) {
		        return nullptr;
		    }
		    if (parent->right.get() == node) {
		        pos -= avl_cnt(node->left.get()) + 1;
		    } else {
		        pos += avl_cnt(node->right.get()) + 1;
		    }
		    node = parent;
		}
	    }
	    return std::unique_ptr<AVLNode>(const_cast<AVLNode*>(node));
	}

} //namespace AVLTree
