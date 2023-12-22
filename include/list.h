#pragma once

#include <cstddef>
#include <memory>

struct DList {
    std::unique_ptr<DList> prev{nullptr};
    std::unique_ptr<DList> next{nullptr};
};

inline void dlist_init(DList* node) {
    node->prev = node;
    node->next = node;
}

inline bool dlist_empty(DList* node) {
    return !node->next;
}

inline void dlist_detach(DList* node) {
    if (node->next) {
        node->next = std::move(node->next->next);
    }
}

inline void dlist_insert_before(DList* target, std::unique_ptr<DList> rookie) {
    rookie->next = std::move(target->next);
    target->next = std::move(rookie);
}

