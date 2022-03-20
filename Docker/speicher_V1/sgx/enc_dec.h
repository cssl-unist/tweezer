#pragma once

#include <string>
#include "openssl/evp.h"
#include "rocksdb/status.h"
#include "rocksdb/slice.h"
#include "rocksdb/flush_block_policy.h"
#include "table/block_based/block.h"
#include "table/format.h"

namespace ROCKSDB_NAMESPACE{

//Will be removed
extern unsigned char gcm_iv[];
extern unsigned char gcm_aad[];
extern unsigned char memtable_key[];

void GenerateKey(std::string FileName, unsigned char* out);

void GenerateMemKey(void);

std::string FilenameToKey(std::string file_name);

void Encryption(Slice data, unsigned char* key, unsigned char* iv, unsigned char* aad, unsigned char* tags = nullptr);

void Decryption(Slice data, unsigned char* key, unsigned char* iv, unsigned char* aad, unsigned char* tags = nullptr);

void log_encrypt (const Slice &record, char* key, char* tags = nullptr, char* aad = nullptr);

void log_decrypt (const Slice &record, char* key, char* tags = nullptr, char* aad = nullptr);

//output should larger than size
void GenerateRandomBytes(char* output,int size);
}

