#pragma once

#include <string>
#include "rocksdb/slice.h"


namespace ROCKSDB_NAMESPACE{

void digest(unsigned char *hmac, const Slice block, std::string sst_key);

}

