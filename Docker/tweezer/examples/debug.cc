// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include <cstdio>
#include <string>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"

using namespace ROCKSDB_NAMESPACE;

#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\rocksdb_simple_example";
#else
std::string kDBPath = "/tmp/rocksdb_simple_example";
#endif

int main() {
  DB* db;
  Options options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  // create the DB if it's not already present
  options.create_if_missing = true;
	options.write_buffer_size=1048576;

  // open DB
  Status s = DB::Open(options, kDBPath, &db);
  assert(s.ok());

  // Put key-value
  s = db->Put(WriteOptions(), "key0", "value");
  assert(s.ok());
  std::string value;
/*
	for(int i = 1 ; i < 100000; i++ ) {
		std::string key = "key";
		std::string n = std::to_string(i);
		key = key + n;
		s = db->Put(WriteOptions(), key, "value");
	}
	for (int i = 0 ; i < 1000; i++) {
    std::string key = "key";
    std::string n = std::to_string(i);
    key = key + n;
  s = db->Get(ReadOptions(), key, &value);
	}
*/
	s = db->Get(ReadOptions(), "key0", &value);
  assert(s.ok());
  assert(value == "value");

  delete db;

  return 0;
}
