#include <cassert>
#include <cstdlib>
#include <memory>
#include "hashtable.h"

using std::size_t;

// n must be a power of 2
void h_init(HTab& htab, size_t n){
	assert(n>0 && (n-1) & n) == 0);
	htab.tab=std::make_unique<std::unique_ptr<HNode>[]>(n):
	htab.mask=n-1;
	htab.size=0;
}

// hashtable insertion
void h_insert(HTab& htab, std::unique_ptr<HNode> node){
	size_t pos=node->hcode & htab.mask;
	node->next=std::move(htab.tab[pos]);
	htab.tab[pos]=std::move(node);
	htab.size++;
}

// hashtable look up subroutine.
// Pay attention to the return value. It returns the address of
// the parent pointer that owns the target node,
// which can be used to delete the target node. 
std::unique_ptr<HNode>* h_lookup(HTab& htab, HNode* key, bool (*cmp)(const HNode*, const HNode*)) {
    if (!htab.tab) {
        return nullptr;
    }

    std::size_t pos = key->hcode & htab.mask;
    auto from = &htab.tab[pos];
    while (*from) {
        if (cmp((*from).get(), key)) {
            return from;
        }
        from = &((*from)->next);
    }
    return nullptr;
}

// remove a node from the chain
std::unique_ptr<HNode> h_detach(HTab& htab, std::unique_ptr<HNode>* from) {
    auto node = std::move(*from);
    *from = std::move(node->next);
    htab.size--;
    return node;
}

constexpr std::size_t k_resizing_work = 128;
constexpr std::size_t k_max_load_factor = 8;

void hm_help_resizing(HMap& hmap) {
    if (!hmap.ht2.tab) {
        return;
    }

    std::size_t nwork = 0;
    while (nwork < k_resizing_work && hmap.ht2.size > 0) {
    	// scan for nodes from ht2 and move them to ht1
        std::size_t pos = hmap.resizing_pos;
        auto& from = hmap.ht2.tab[pos];
        if (!from) {
            hmap.resizing_pos = (pos + 1) % (hmap.ht2.mask + 1);
            continue;
        }

        h_insert(hmap.ht1, h_detach(hmap.ht2, &from));
        nwork++;
    }

    if (hmap.ht2.size == 0) {
        hmap.ht2 = HTab{};
    }
}

void hm_start_resizing(HMap& hmap) {
    assert(!hmap.ht2.tab);
    // create a bigger hashtable and swap them
    hmap.ht2 = std::move(hmap.ht1);
    h_init(hmap.ht1, (hmap.ht1.mask + 1) * 2);
    hmap.resizing_pos = 0;
}

HNode* hm_lookup(HMap& hmap, HNode* key, bool (*cmp)(const HNode*, const HNode*)) {
    hm_help_resizing(hmap);
    auto from = h_lookup(hmap.ht1, key, cmp);
    if (!from) {
        from = h_lookup(hmap.ht2, key, cmp);
    }
    return from ? from->get() : nullptr;
}

void hm_insert(HMap& hmap, std::unique_ptr<HNode> node) {
    if (!hmap.ht1.tab) {
        h_init(hmap.ht1, 4);
    }
    h_insert(hmap.ht1, std::move(node));

    if (!hmap.ht2.tab) {
    // check whether we need to resize
        std::size_t load_factor = hmap.ht1.size / (hmap.ht1.mask + 1);
        if (load_factor >= k_max_load_factor) {
            hm_start_resizing(hmap);
        }
    }
    hm_help_resizing(hmap);
}

std::unique_ptr<HNode> hm_pop(HMap& hmap, HNode* key, bool (*cmp)(const HNode*, const HNode*)) {
    hm_help_resizing(hmap);
    auto from = h_lookup(hmap.ht1, key, cmp);
    if (from) {
        return h_detach(hmap.ht1, from);
    }
    from = h_lookup(hmap.ht2, key, cmp);
    if (from) {
        return h_detach(hmap.ht2, from);
    }
    return nullptr;
}

std::size_t hm_size(HMap& hmap) {
    return hmap.ht1.size + hmap.ht2.size;
}

void hm_destroy(HMap& hmap) {
    hmap.ht1 = HTab{};
    hmap.ht2 = HTab{};
}


