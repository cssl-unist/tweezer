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


void digest(unsigned char* hmac, const Slice block,std::string sst_key){
	const unsigned char* key = (const unsigned char*)sst_key.data();
//Solving Memory corruption
	unsigned int n;
	HMAC(EVP_sha3_384(), key, 32,(const unsigned char*) block.data(), block.size(), hmac,&n);

}

}
