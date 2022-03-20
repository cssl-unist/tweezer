#pragma once
#include "table/block_based/block.h"
#include "table/format.h"
#include "sgx/enc_dec.h"
#include "sgx/hmac.h"
#include "db/dbformat.h"
#include "util/mutexlock.h"

namespace ROCKSDB_NAMESPACE {
class EncryptedDataBlockIter;
struct DecodeEntryV2 {
  inline const char* operator()(const char* p, const char* limit,
                                uint32_t* shared, uint32_t* non_shared,
                                uint32_t* value_length) {
    // We need 2 bytes for shared and non_shared size. We also need one more
    // byte either for value size or the actual value in case of value delta
    // encoding.
    assert(limit - p >= 3);
    *shared = reinterpret_cast<const unsigned char*>(p)[0];
    *non_shared = reinterpret_cast<const unsigned char*>(p)[1];
    *value_length = reinterpret_cast<const unsigned char*>(p)[2];
    if ((*shared | *non_shared | *value_length) < 128) {
      // Fast path: all three values are encoded in one byte each
      p += 3;
    } else {
      if ((p = GetVarint32Ptr(p, limit, shared)) == nullptr) return nullptr;
      if ((p = GetVarint32Ptr(p, limit, non_shared)) == nullptr) return nullptr;
      if ((p = GetVarint32Ptr(p, limit, value_length)) == nullptr) {
        return nullptr;
      }
    }

    // Using an assert in place of "return null" since we should not pay the
    // cost of checking for corruption on every single key decoding
    //assert(!(static_cast<uint32_t>(limit - p) < (*non_shared + *value_length)));
    return p;
  }
};


class EncryptedBlock : public Block {
 public:
  explicit EncryptedBlock(BlockContents&& contents, size_t read_amp_bytes_per_bit = 0,
                 Statistics* statistics = nullptr);
  // No copying allowed
  EncryptedBlock(const EncryptedBlock&) = delete;
  void operator=(const EncryptedBlock&) = delete;

  ~EncryptedBlock();

  EncryptedDataBlockIter* NewDataIterator(const Comparator* raw_ucmp,
                                 SequenceNumber global_seqno,
                                 EncryptedDataBlockIter* iter = nullptr,
                                 Statistics* stats = nullptr,
                                 bool block_contents_pinned = false);
	void Set(std::string sst_key_) { 
    MutexLock mutex(&mu);
    sst_key = sst_key_;
  }

 private:
	std::string sst_key;
  port::Mutex mu;

};

class EncryptedDataBlockIter final : public DataBlockIter {
 public:
  EncryptedDataBlockIter()
      : DataBlockIter(), read_amp_bitmap_(nullptr), last_bitmap_offset_(0) {}
  EncryptedDataBlockIter(const Comparator* raw_ucmp, const char* data, uint32_t restarts,
                uint32_t num_restarts, SequenceNumber global_seqno,
                BlockReadAmpBitmap* read_amp_bitmap, bool block_contents_pinned,
                DataBlockHashIndex* data_block_hash_index)
      : DataBlockIter() {
    Initialize(raw_ucmp, data, restarts, num_restarts, global_seqno,
               read_amp_bitmap, block_contents_pinned, data_block_hash_index);
  }

	struct offset_entry {
		offset_entry(std::string key_, uint32_t offset_, uint32_t size_)
		: key(key_), offset(offset_), size(size_) {}
		std::string key;
		uint32_t offset;
		uint32_t size;
	};

  void Initialize(const Comparator* raw_ucmp, const char* data,
                  uint32_t restarts, uint32_t num_restarts,
                  SequenceNumber global_seqno,
                  BlockReadAmpBitmap* read_amp_bitmap,
                  bool block_contents_pinned,
                  DataBlockHashIndex* data_block_hash_index) {
    InitializeBase(raw_ucmp, data, restarts, num_restarts, global_seqno,
                   block_contents_pinned);
    raw_key_.SetIsUserKey(false);
    read_amp_bitmap_ = read_amp_bitmap;
    last_bitmap_offset_ = current_ + 1;
    data_block_hash_index_ = data_block_hash_index;
  }

	void Ready(std::string sst_key_, size_t size_) {
		key_offset_map.clear();
		sst_key = sst_key_;
		uint32_t meta_offset = DecodeFixed32(data_ + size_ - sizeof(uint32_t));
		restarts_ = meta_offset;
		uint32_t length = size_ - meta_offset - sizeof(uint32_t) /*meta_offset*/;
		char *enc_buf = new char[length];
		memcpy(enc_buf, data_ + meta_offset,length); 
		Decryption(Slice(enc_buf,length), (unsigned char*)sst_key.data(),gcm_iv,gcm_aad);
		IterKey entry_key;
		uint32_t offset = 0;
		char* current_ptr_ = enc_buf;
		while(true) {
			if (current_ptr_ >= enc_buf + length)
				break;
			uint32_t shared, non_shared, value_length;
			char *next_;
			next_ = (char*)DecodeEntryV2()(current_ptr_,enc_buf + length, &shared, &non_shared, &value_length);
			if (shared == 0) {
				entry_key.SetKey(Slice(next_,non_shared), true);
			} else {
				entry_key.TrimAppend(shared, next_, non_shared);
			}
			Slice prev_key;
			Slice curr_key;
			if (key_offset_map.size() > 0) {
				curr_key = entry_key.GetKey();	
				prev_key = Slice(key_offset_map[key_offset_map.size()-1].key);
				curr_key = ExtractUserKey(curr_key);
				prev_key = ExtractUserKey(prev_key);
				if (ucmp().Compare(prev_key,curr_key) > 0) {
					fprintf(stdout,"Block corruption occur during reading key block \n");
					exit(-1);
				}
			}
			std::string new_key(entry_key.GetKey().data(), entry_key.GetKey().size());
			key_offset_map.emplace_back(offset_entry(new_key,offset,value_length));	
			//Calculate next offset
			offset += value_length;
			offset += 48;
			current_ptr_ = next_ + non_shared;
		}
		current_index = -1;
		delete [] enc_buf;

	}

	Slice first_key(){
		return Slice(key_offset_map[0].key);
	}
	Slice last_key(){
		return Slice(key_offset_map[key_offset_map.size()-1].key);
	}
	void Updatevalue();

  Slice value() const override {
    assert(Valid());
    if (read_amp_bitmap_ && current_ < restarts_ &&
        current_ != last_bitmap_offset_) {
      read_amp_bitmap_->Mark(current_ /* current entry offset */,
                             NextEntryOffset() - 1);
      last_bitmap_offset_ = current_;
    }
		if (value_dirty)
			const_cast<EncryptedDataBlockIter*>(this)->Updatevalue();
    return value_;
  }

  inline bool SeekForGet(const Slice& target) {
    if (!data_block_hash_index_) {
      SeekImpl(target);
      UpdateKey();
      return true;
    }
    bool res = SeekForGetImpl(target);
    UpdateKey();
    return res;
  }

  void Invalidate(Status s) {
    InvalidateBase(s);
  }

 protected:
  virtual void SeekToFirstImpl() override;
  virtual void SeekToLastImpl() override;
  virtual void SeekImpl(const Slice& target) override;
  virtual void SeekForPrevImpl(const Slice& target) override;
  virtual void NextImpl() override;
  virtual void PrevImpl() override;

 private:
  // read-amp bitmap
  BlockReadAmpBitmap* read_amp_bitmap_;
  // last `current_` value we report to read-amp bitmp
  mutable uint32_t last_bitmap_offset_;

  DataBlockHashIndex* data_block_hash_index_;

  bool SeekForGetImpl(const Slice& target);

	std::vector<offset_entry> key_offset_map;
	int current_index;
	std::string sst_key;
	std::unique_ptr<char[]> buff;
	//Check value_ is dirty
	bool value_dirty = true;
};


}


