#include <string>
#include <map>
#include "rocksdb/slice.h"
#include "openssl/hmac.h"
#include "openssl/evp.h"
#include "sgx/enc_dec.h"
#include "sgx/hmac.h"
#include "file/filename.h"

namespace ROCKSDB_NAMESPACE{

extern std::map<uint64_t, std::string> KeyList;
extern pthread_rwlock_t key_lock;


void digest(unsigned char* hmac, const Slice block,unsigned char *sst_key){
//Solving Memory corruption
	unsigned int n;
	HMAC(EVP_sha3_384(), sst_key, 32,(const unsigned char*) block.data(), block.size(), hmac,&n);

}

}
