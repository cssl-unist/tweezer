#include "sgx/encryptedblock.h"
#include "table/format.h"
#include "table/block_based/block_based_table_reader.h"
#include "table/block_based/reader_common.h"

namespace ROCKSDB_NAMESPACE {

EncryptedBlock::~EncryptedBlock() {
  // This sync point can be re-enabled if RocksDB can control the
  // initialization order of any/all static options created by the user.
  // TEST_SYNC_POINT("Block::~Block");
}

EncryptedBlock::EncryptedBlock(BlockContents&& contents, size_t read_amp_bytes_per_bit,
             Statistics* statistics) 
		: Block(std::move(contents),true, read_amp_bytes_per_bit, statistics){ 
	
//We disable restart block. One block = One restart Block
  TEST_SYNC_POINT("Block::Block:0");
  if (size_ < sizeof(uint32_t)) {
    size_ = 0;  // Error marker
  }
  if (read_amp_bytes_per_bit != 0 && statistics && size_ != 0) {
    read_amp_bitmap_.reset(new BlockReadAmpBitmap(
        restart_offset_, read_amp_bytes_per_bit, statistics));
  }
}

//Need change
EncryptedDataBlockIter* EncryptedBlock::NewDataIterator(const Comparator* raw_ucmp,
                                      SequenceNumber global_seqno,
                                      EncryptedDataBlockIter* iter, Statistics* stats,
                                      __attribute__((unused)) bool block_contents_pinned) {
  EncryptedDataBlockIter* ret_iter;
  if (iter != nullptr) {
    ret_iter = iter;
  } else {
    ret_iter = new EncryptedDataBlockIter;
  }
  if (size_ < 2 * sizeof(uint32_t)) {
    ret_iter->Invalidate(Status::Corruption("bad block contents"));
    return ret_iter;
  }
  if (num_restarts_ == 0) {
    // Empty block.
    ret_iter->Invalidate(Status::OK());
    return ret_iter;
  } else {
    ret_iter->Initialize(
        raw_ucmp, data_, restart_offset_, num_restarts_, global_seqno,
        read_amp_bitmap_.get(), false/*block_contents_pinned*/, nullptr);
/*Make copy of file_name to thread local = stack */
		ret_iter->Ready(sst_key,size_);
    if (read_amp_bitmap_) {
      if (read_amp_bitmap_->GetStatistics() != stats) {
        // DB changed the Statistics pointer, we need to notify read_amp_bitmap_
        read_amp_bitmap_->SetStatistics(stats);
      }
    }
  }

  return ret_iter;
}

void EncryptedDataBlockIter::Updatevalue() {
	uint32_t value_length = key_offset_map[current_index].size;
	uint32_t offset = key_offset_map[current_index].offset;
	buff.reset(new char[value_length]);
	memcpy(buff.get(),data_ + offset, value_length);
	value_ = Slice(buff.get(),value_length);
	char *hmac_buf = new char[key_.size() + value_.size()];
	memcpy(hmac_buf,key_.data(), key_.size());
	memcpy(hmac_buf + key_.size(),value_.data(), value_.size());
	char hmacfromvalue[48];
	digest((unsigned char*)hmacfromvalue, Slice(hmac_buf,key_.size() + value_.size()), sst_key);
	Slice hmac_s(hmacfromvalue,48);
	char hmacfromfile[48];
	memcpy(hmacfromfile,data_ + key_offset_map[current_index].offset + value_.size(), 48);
	if (hmac_s.compare(Slice(hmacfromfile,48))) {
		fprintf(stdout,"Block verification fail \n");
		exit(-1);
	}
	delete [] hmac_buf;
	Decryption(value_,(unsigned char*)sst_key.data(),gcm_iv,gcm_aad);
	value_dirty = false;
}

void EncryptedDataBlockIter::NextImpl() { 
	if (current_index < static_cast<int>(key_offset_map.size()) - 1) {
		current_index += 1;
	}
	else {
//To make Valid() fail
		current_ = current_ + 20000;
		return;
	}
	raw_key_.SetKey(Slice(key_offset_map[current_index].key));
	current_ = key_offset_map[current_index].offset + 48;
	value_dirty = true;
}


void EncryptedDataBlockIter::PrevImpl() {
	if(current_index > 0) {
		current_index -= 1;
	}
  raw_key_.SetKey(Slice(key_offset_map[current_index].key));
  current_ = key_offset_map[current_index].offset + 48;
	value_dirty = true;
}

void EncryptedDataBlockIter::SeekImpl(const Slice& target) {
//Reduce seek
	int kRange = 15;
	int start = 0;
	int end = key_offset_map.size();
	while(true) {
		int middle = (start + end) / 2;
		Slice key = Slice(key_offset_map[middle].key);
		current_index = middle;
		raw_key_.SetKey(key);
		int res = CompareCurrentKey(key);
		if (res < 0) {
			start = middle;
		}	else {
			int range = middle - start;
			if(range <= kRange) {
				current_index = start - 1;
				break;
			}
			end = middle - 1;
		}
	}
	NextImpl();
	for (; current_index < static_cast<int>(key_offset_map.size()) && Valid(); NextImpl()) {
		if (CompareCurrentKey(target) >= 0)
			break;
	}
}

bool EncryptedDataBlockIter::SeekForGetImpl(const Slice& target) {
	NextImpl();
	for (; current_index < static_cast<int>(key_offset_map.size()) && Valid(); NextImpl()) {
		if (CompareCurrentKey(target) >= 0)
		break;
  	}
  	return true;
}

void EncryptedDataBlockIter::SeekForPrevImpl(const Slice& target) {
  PERF_TIMER_GUARD(block_seek_nanos);
  Slice seek_key = target;
	SeekImpl(target);

  if (!Valid()) {
    SeekToLastImpl();
  } else {
    while (Valid() && CompareCurrentKey(seek_key) > 0) {
      PrevImpl();
    }
  }
}

void EncryptedDataBlockIter::SeekToFirstImpl() {
  if (data_ == nullptr) {  // Not init yet
    return;
  }
	current_index = -1;
	NextImpl();
}

void EncryptedDataBlockIter::SeekToLastImpl() {
  if (data_ == nullptr) {  // Not init yet
    return;
  }
	current_index = key_offset_map.size()+1;
	PrevImpl();
}

}
