// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include <cstdio>
#include <string>
#include <iostream>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"

using namespace ROCKSDB_NAMESPACE;

std::string kDBPath = "/tmp/flush_test";

int main() {
  DB* db;
  Options options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  // create the DB if it's not already present
  options.create_if_missing = true;

  // open DB
  Status s = DB::Open(options, kDBPath, &db);
	

  // Put key-value
	for (int i = 1000; i < 1000000; ++i) {
    s = db->Put(WriteOptions(), std::to_string(i),
                            std::string(500, 'a' + (i % 26)));

  }
	std::cout << "Put done\n";
	std::string value;
	s = db->Get(ReadOptions(), std::to_string(1000),&value);
	std::cout << "Read Value : " << value << std::endl;
	assert(s.ok());
	


  return 0;
}
