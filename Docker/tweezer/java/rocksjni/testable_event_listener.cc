#include <climits>
#include <utility>

#include "include/org_rocksdb_test_TestableEventListener.h"
#include "rocksdb/listener.h"

using namespace ROCKSDB_NAMESPACE;

static TableProperties newTablePropertiesForTest() {
  TableProperties table_properties;
  table_properties.data_size = LLONG_MAX;
  table_properties.index_size = LLONG_MAX;
  table_properties.index_partitions = LLONG_MAX;
  table_properties.top_level_index_size = LLONG_MAX;
  table_properties.index_key_is_user_key = LLONG_MAX;
  table_properties.index_value_is_delta_encoded = LLONG_MAX;
  table_properties.filter_size = LLONG_MAX;
  table_properties.raw_key_size = LLONG_MAX;
  table_properties.raw_value_size = LLONG_MAX;
  table_properties.num_data_blocks = LLONG_MAX;
  table_properties.num_entries = LLONG_MAX;
  table_properties.num_deletions = LLONG_MAX;
  table_properties.num_merge_operands = LLONG_MAX;
  table_properties.num_range_deletions = LLONG_MAX;
  table_properties.format_version = LLONG_MAX;
  table_properties.fixed_key_len = LLONG_MAX;
  table_properties.column_family_id = LLONG_MAX;
  table_properties.creation_time = LLONG_MAX;
  table_properties.oldest_key_time = LLONG_MAX;
  table_properties.file_creation_time = LLONG_MAX;
  table_properties.db_id = "dbId";
  table_properties.db_session_id = "sessionId";
  table_properties.column_family_name = "columnFamilyName";
  table_properties.filter_policy_name = "filterPolicyName";
  table_properties.comparator_name = "comparatorName";
  table_properties.merge_operator_name = "mergeOperatorName";
  table_properties.prefix_extractor_name = "prefixExtractorName";
  table_properties.property_collectors_names = "propertyCollectorsNames";
  table_properties.compression_name = "compressionName";
  table_properties.compression_options = "compressionOptions";
  table_properties.user_collected_properties = {{"key", "value"}};
  table_properties.readable_properties = {{"key", "value"}};
  table_properties.properties_offsets = {{"key", LLONG_MAX}};
  return table_properties;
}

/*
 * Class:     org_rocksdb_test_TestableEventListener
 * Method:    invokeAllCallbacks
 * Signature: (J)V
 */
void Java_org_rocksdb_test_TestableEventListener_invokeAllCallbacks(
    JNIEnv *, jclass, jlong jhandle) {
  const auto &el =
      *reinterpret_cast<std::shared_ptr<ROCKSDB_NAMESPACE::EventListener> *>(
          jhandle);

  TableProperties table_properties = newTablePropertiesForTest();

  FlushJobInfo flush_job_info;
  flush_job_info.cf_id = INT_MAX;
  flush_job_info.cf_name = "testColumnFamily";
  flush_job_info.file_path = "/file/path";
  flush_job_info.file_number = LLONG_MAX;
  flush_job_info.oldest_blob_file_number = LLONG_MAX;
  flush_job_info.thread_id = LLONG_MAX;
  flush_job_info.job_id = INT_MAX;
  flush_job_info.triggered_writes_slowdown = true;
  flush_job_info.triggered_writes_stop = true;
  flush_job_info.smallest_seqno = LLONG_MAX;
  flush_job_info.largest_seqno = LLONG_MAX;
  flush_job_info.table_properties = table_properties;
  flush_job_info.flush_reason = FlushReason::kManualFlush;

  el->OnFlushCompleted(nullptr, flush_job_info);
  el->OnFlushBegin(nullptr, flush_job_info);

  Status status = Status::Incomplete(Status::SubCode::kNoSpace);

  TableFileDeletionInfo file_deletion_info;
  file_deletion_info.db_name = "dbName";
  file_deletion_info.file_path = "/file/path";
  file_deletion_info.job_id = INT_MAX;
  file_deletion_info.status = status;

  el->OnTableFileDeleted(file_deletion_info);

  CompactionJobInfo compaction_job_info;
  compaction_job_info.cf_id = INT_MAX;
  compaction_job_info.cf_name = "compactionColumnFamily";
  compaction_job_info.status = status;
  compaction_job_info.thread_id = LLONG_MAX;
  compaction_job_info.job_id = INT_MAX;
  compaction_job_info.base_input_level = INT_MAX;
  compaction_job_info.output_level = INT_MAX;
  compaction_job_info.input_files = {"inputFile.sst"};
  compaction_job_info.input_file_infos = {};
  compaction_job_info.output_files = {"outputFile.sst"};
  compaction_job_info.output_file_infos = {};
  compaction_job_info.table_properties = {
      {"tableProperties", std::shared_ptr<TableProperties>(
                              &table_properties, [](TableProperties *) {})}};
  compaction_job_info.compaction_reason = CompactionReason::kFlush;
  compaction_job_info.compression = CompressionType::kSnappyCompression;

  compaction_job_info.stats = CompactionJobStats();

  el->OnCompactionBegin(nullptr, compaction_job_info);
  el->OnCompactionCompleted(nullptr, compaction_job_info);

  TableFileCreationInfo file_creation_info;
  file_creation_info.file_size = LLONG_MAX;
  file_creation_info.table_properties = table_properties;
  file_creation_info.status = status;
  file_creation_info.file_checksum = "fileChecksum";
  file_creation_info.file_checksum_func_name = "fileChecksumFuncName";
  file_creation_info.db_name = "dbName";
  file_creation_info.cf_name = "columnFamilyName";
  file_creation_info.file_path = "/file/path";
  file_creation_info.job_id = INT_MAX;
  file_creation_info.reason = TableFileCreationReason::kMisc;

  el->OnTableFileCreated(file_creation_info);

  TableFileCreationBriefInfo file_creation_brief_info;
  file_creation_brief_info.db_name = "dbName";
  file_creation_brief_info.cf_name = "columnFamilyName";
  file_creation_brief_info.file_path = "/file/path";
  file_creation_brief_info.job_id = INT_MAX;
  file_creation_brief_info.reason = TableFileCreationReason::kMisc;

  el->OnTableFileCreationStarted(file_creation_brief_info);

  MemTableInfo mem_table_info;
  mem_table_info.cf_name = "columnFamilyName";
  mem_table_info.first_seqno = LLONG_MAX;
  mem_table_info.earliest_seqno = LLONG_MAX;
  mem_table_info.num_entries = LLONG_MAX;
  mem_table_info.num_deletes = LLONG_MAX;

  el->OnMemTableSealed(mem_table_info);
  el->OnColumnFamilyHandleDeletionStarted(nullptr);

  ExternalFileIngestionInfo file_ingestion_info;
  file_ingestion_info.cf_name = "columnFamilyName";
  file_ingestion_info.external_file_path = "/external/file/path";
  file_ingestion_info.internal_file_path = "/internal/file/path";
  file_ingestion_info.global_seqno = LLONG_MAX;
  file_ingestion_info.table_properties = table_properties;
  el->OnExternalFileIngested(nullptr, file_ingestion_info);

  el->OnBackgroundError(BackgroundErrorReason::kFlush, &status);

  WriteStallInfo write_stall_info;
  write_stall_info.cf_name = "columnFamilyName";
  write_stall_info.condition.cur = WriteStallCondition::kDelayed;
  write_stall_info.condition.prev = WriteStallCondition::kStopped;
  el->OnStallConditionsChanged(write_stall_info);

  FileOperationInfo op_info = FileOperationInfo(
      FileOperationType::kRead, "/file/path",
      std::make_pair(std::chrono::time_point<std::chrono::system_clock,
                                             std::chrono::nanoseconds>(
                         std::chrono::nanoseconds(1600699420000000000ll)),
                     std::chrono::time_point<std::chrono::steady_clock,
                                             std::chrono::nanoseconds>(
                         std::chrono::nanoseconds(1600699420000000000ll))),
      std::chrono::time_point<std::chrono::steady_clock,
                              std::chrono::nanoseconds>(
          std::chrono::nanoseconds(1600699425000000000ll)),
      status);
  op_info.offset = LLONG_MAX;
  op_info.length = LLONG_MAX;
  op_info.status = status;

  el->OnFileReadFinish(op_info);
  el->OnFileWriteFinish(op_info);
  el->OnFileFlushFinish(op_info);
  el->OnFileSyncFinish(op_info);
  el->OnFileRangeSyncFinish(op_info);
  el->OnFileTruncateFinish(op_info);
  el->OnFileCloseFinish(op_info);
  el->ShouldBeNotifiedOnFileIO();

  bool auto_recovery;
  el->OnErrorRecoveryBegin(BackgroundErrorReason::kFlush, status,
                           &auto_recovery);
  el->OnErrorRecoveryCompleted(status);
}
