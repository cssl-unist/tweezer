// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include "rocksdb/status.h"
#include "rocksdb/memory_allocator.h"
#include "memory/memory_allocator.h"
#ifdef MEMTABLE
#include "gperftools/tcmalloc.h"
#endif

#include <memory>

namespace ROCKSDB_NAMESPACE {

// All methods should be thread-safe.
class UntrustedMemoryAllocator : public MemoryAllocator {
 public:
	UntrustedMemoryAllocator(){}
	~UntrustedMemoryAllocator(){}

  // Name of the cache allocator, printed in the log
  const char* Name() const override { return "UntrusedTCMALLOCAllocator";}

  // Allocate a block of at least size. Has to be thread-safe.
  void* Allocate(size_t size) override {
    void* ret = tc_malloc(size);
    return ret;
  }


  // Deallocate previously allocated block. Has to be thread-safe.
  void Deallocate(void* p) override {
    tc_free(p);
  }

  size_t UsableSize(void* /*p*/, size_t allocation_size) const override{
    // default implementation just returns the allocation size
    return allocation_size;
  }
};
extern UntrustedMemoryAllocator *untrusted_allocator;

inline CacheAllocationPtr UntrustedAllocateBlock(size_t size) {
	auto block = reinterpret_cast<char*>(untrusted_allocator->Allocate(size));
	return CacheAllocationPtr(block, untrusted_allocator);
}

}

