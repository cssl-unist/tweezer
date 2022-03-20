//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "memory/untrusted_arena.h"
#include "memory/arena.h"
#include <algorithm>
#include "logging/logging.h"
#include "port/malloc.h"
#include "port/port.h"
#include "rocksdb/env.h"
#include "test_util/sync_point.h"
#include "fcntl.h"
#include "unistd.h"
#include "gperftools/tcmalloc.h"

namespace ROCKSDB_NAMESPACE {


/*
const size_t Arena::kMinBlockSize = 4096;
const size_t Arena::kMaxBlockSize = 2u << 30;
 */
//Untrusted Arena size : 4096B Fixed
const size_t Untrusted_Arena::kMinBlockSize = 4096;
const size_t Untrusted_Arena::kMaxBlockSize = 2u << 30;

static const int kAlignUnit = alignof(max_align_t);

Untrusted_Arena::Untrusted_Arena(size_t block_size, AllocTracker* tracker, size_t huge_page_size)
    : kBlockSize(OptimizeBlockSize(block_size)), tracker_(tracker) {
  assert(kBlockSize >= kMinBlockSize && kBlockSize <= kMaxBlockSize &&
         kBlockSize % kAlignUnit == 0);
  TEST_SYNC_POINT_CALLBACK("Arena::Arena:0", const_cast<size_t*>(&kBlockSize));
  alloc_bytes_remaining_ = 0;
  blocks_memory_ += alloc_bytes_remaining_;
  aligned_alloc_ptr_ = nullptr;
  unaligned_alloc_ptr_ = nullptr;
  (void)huge_page_size;
	
  if (tracker_ != nullptr) {
    tracker_->Allocate(kInlineSize);
  }
}

Untrusted_Arena::~Untrusted_Arena() {
  if (tracker_ != nullptr) {
    assert(tracker_->is_freed());
    tracker_->FreeMem();
  }
	for (const auto& block : blocks_) {
		tc_free(block);
	}

}

char* Untrusted_Arena::Allocate(size_t bytes) {
  // The semantics of what to return are a bit messy if we allow
  // 0-byte allocations, so we disallow them here (we don't need
  // them for our internal use).
  assert(bytes > 0);
  if (bytes <= alloc_bytes_remaining_) {
    unaligned_alloc_ptr_ -= bytes;
    alloc_bytes_remaining_ -= bytes;
    return unaligned_alloc_ptr_;
  }
  return AllocateFallback(bytes, false /* unaligned */);
}


char* Untrusted_Arena::AllocateFallback(size_t bytes, bool aligned) {
  if (bytes > kBlockSize) {
    fprintf(stdout,"Fail larger than 4KB cannot be allocated \n");
		exit(-1);
  }

  // We waste the remaining space in the current block.
  size_t size = 0;
  char* block_head = nullptr;
  if (!block_head) {
    size = kBlockSize;
    block_head = AllocateNewBlock(size);
  }
  alloc_bytes_remaining_ = size - bytes;

  if (aligned) {
    aligned_alloc_ptr_ = block_head + bytes;
    unaligned_alloc_ptr_ = block_head + size;
    return block_head;
  } else {
    aligned_alloc_ptr_ = block_head;
    unaligned_alloc_ptr_ = block_head + size - bytes;
    return unaligned_alloc_ptr_;
  }
}

char* Untrusted_Arena::AllocateAligned(size_t bytes, size_t huge_page_size,
                             Logger* logger) {
  assert((kAlignUnit & (kAlignUnit - 1)) ==
         0);  // Pointer size should be a power of 2
  (void)huge_page_size;
  (void)logger;

  size_t current_mod =
      reinterpret_cast<uintptr_t>(aligned_alloc_ptr_) & (kAlignUnit - 1);
  size_t slop = (current_mod == 0 ? 0 : kAlignUnit - current_mod);
  size_t needed = bytes + slop;
  char* result;
  if (needed <= alloc_bytes_remaining_) {
    result = aligned_alloc_ptr_ + slop;
    aligned_alloc_ptr_ += needed;
    alloc_bytes_remaining_ -= needed;
  } else {
    // AllocateFallback always returns aligned memory
    result = AllocateFallback(bytes, true /* aligned */);
  }
  assert((reinterpret_cast<uintptr_t>(result) & (kAlignUnit - 1)) == 0);
  return result;
}

char* Untrusted_Arena::AllocateNewBlock(size_t block_bytes) {
  // Reserve space in `blocks_` before allocating memory via new.
  // Use `emplace_back()` instead of `reserve()` to let std::vector manage its
  // own memory and do fewer reallocations.
  //
  // - If `emplace_back` throws, no memory leaks because we haven't called `new`
  //   yet.
  // - If `new` throws, no memory leaks because the vector will be cleaned up
  //   via RAII.
  blocks_.emplace_back(nullptr);

//Replace
  char* block = reinterpret_cast<char*>(tc_malloc(block_bytes));
  //char* block = reinterpret_cast<char*>(malloc(block_bytes));
  size_t allocated_size;
  allocated_size = block_bytes;
  blocks_memory_ += allocated_size;
  if (tracker_ != nullptr) {
    tracker_->Allocate(allocated_size);
  }
  blocks_.back() = block;
  return block;
}

}  // namespace ROCKSDB_NAMESPACE
