#pragma once

#include <string>
#include "rocksdb/slice.h"
#include "enc_dec.h"


namespace ROCKSDB_NAMESPACE{

void digest(unsigned char *hmac, const Slice block, unsigned char *sst_key);

}

