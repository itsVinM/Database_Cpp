#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

struct HeapItem{
	uint64_t val{0};
	std::unique_ptr<HeapItem> ref{nullptr};
}

void heap_update(HeapItem& a, std::size_t pos, std::size_t len); 
