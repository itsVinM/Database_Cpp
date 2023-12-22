#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <functional>

using std::size_t;

//hashtable node, should be embedded into the payload
struct HNode{
	std::unique_ptr<HNode> next=nullptr;
	uint64_t hcode{0};
}

// simple & fixed-sized hastable
struct HTab{
	std::unique_ptr<HNode[]> tab=nullptr;
	size_t mask{0}, size{0};
}

// real hastable interface, it uses 2 hashtables for progressive resizing
struct HMap{
	HTab ht1;
	HTab ht2;
	size_t resizing_pos{0};
};


HNode* hm_lookup(HMap* hmap, HNode* key, bool (*cmp)(const HNode*, const HNode*));
void hm_insert(HMap* hmap, std::unique_ptr<HNode> node);
HNode* hm_pop(HMap* hmap, const HNode* key, bool (*cmp)(const HNode*, const HNode*));
std::size_t hm_size(HMap* hmap);
void hm_destroy(HMap* hmap);

