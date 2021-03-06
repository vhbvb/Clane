//===- IndexStore.cpp - Index store API -----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the API for the index store.
//
//===----------------------------------------------------------------------===//

#include "indexstore/indexstore.h"
#include "clang/Index/IndexDataStore.h"
#include "clang/Index/IndexDataStoreSymbolUtils.h"
#include "clang/Index/IndexRecordReader.h"
#include "clang/Index/IndexUnitReader.h"
#include "clang/Index/IndexUnitWriter.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/TimeValue.h"
#include <Block.h>

using namespace clang;
using namespace clang::index;
using namespace llvm;

static indexstore_string_ref_t toIndexStoreString(StringRef str) {
  return indexstore_string_ref_t{ str.data(), str.size() };
}

namespace {

struct IndexStoreError {
  std::string Error;
};

} // anonymous namespace

const char *
indexstore_error_get_description(indexstore_error_t err) {
  return static_cast<IndexStoreError*>(err)->Error.c_str();
}

void
indexstore_error_dispose(indexstore_error_t err) {
  delete static_cast<IndexStoreError*>(err);
}

unsigned
indexstore_format_version(void) {
  return IndexDataStore::getFormatVersion();
}

indexstore_t
indexstore_store_create(const char *store_path, indexstore_error_t *c_error) {
  std::unique_ptr<IndexDataStore> store;
  std::string error;
  store = IndexDataStore::create(store_path, error);
  if (!store) {
    if (c_error)
      *c_error = new IndexStoreError{ error };
    return nullptr;
  }
  return store.release();
}

void
indexstore_store_dispose(indexstore_t store) {
  delete static_cast<IndexDataStore*>(store);
}

#if INDEXSTORE_HAS_BLOCKS
bool
indexstore_store_units_apply(indexstore_t c_store, unsigned sorted,
                            bool(^applier)(indexstore_string_ref_t unit_name)) {
  IndexDataStore *store = static_cast<IndexDataStore*>(c_store);
  return store->foreachUnitName(sorted, [&](StringRef unitName) -> bool {
    return applier(toIndexStoreString(unitName));
  });
}

bool
indexstore_store_set_unit_event_handler(indexstore_t c_store,
                                    indexstore_unit_event_handler_t blk_handler,
                                    indexstore_error_t *c_error) {
  IndexDataStore *store = static_cast<IndexDataStore*>(c_store);
  if (!blk_handler) {
    std::string error;
    bool err = store->setUnitEventHandler(nullptr, error);
    if (err && c_error)
      *c_error = new IndexStoreError{ error };
    return err;
  }

  class BlockWrapper {
    indexstore_unit_event_handler_t blk_handler;
  public:
    BlockWrapper(indexstore_unit_event_handler_t handler) {
      blk_handler = Block_copy(handler);
    }
    BlockWrapper(const BlockWrapper &other) {
      blk_handler = Block_copy(other.blk_handler);
    }
    ~BlockWrapper() {
      Block_release(blk_handler);
    }

    void operator()(indexstore_unit_event_t *events, size_t count) const {
      blk_handler(events, count);
    }
  };

  BlockWrapper handler(blk_handler);

  std::string error;
  bool err = store->setUnitEventHandler([handler](ArrayRef<IndexDataStore::UnitEvent> events) {
    SmallVector<indexstore_unit_event_t, 16> store_events;
    store_events.reserve(events.size());
    for (const IndexDataStore::UnitEvent &evt : events) {
      indexstore_unit_event_kind_t k;
      switch (evt.Kind) {
      case IndexDataStore::UnitEventKind::Added:
        k = INDEXSTORE_UNIT_EVENT_ADDED; break;
      case IndexDataStore::UnitEventKind::Removed:
        k = INDEXSTORE_UNIT_EVENT_REMOVED; break;
      case IndexDataStore::UnitEventKind::Modified:
        k = INDEXSTORE_UNIT_EVENT_MODIFIED; break;
      case IndexDataStore::UnitEventKind::DirectoryDeleted:
        k = INDEXSTORE_UNIT_EVENT_DIRECTORY_DELETED; break;
      }
      store_events.push_back(indexstore_unit_event_t{k, toIndexStoreString(evt.UnitName)});
    }

    handler(store_events.data(), store_events.size());
  }, error);

  if (err && c_error)
    *c_error = new IndexStoreError{ error };
  return err;
}
#endif

void
indexstore_store_discard_unit(indexstore_t c_store, const char *unit_name) {
  IndexDataStore *store = static_cast<IndexDataStore*>(c_store);
  store->discardUnit(unit_name);
}

void
indexstore_store_purge_stale_records(indexstore_t c_store,
                                     indexstore_string_ref_t *active_record_names,
                                     size_t active_record_count) {
  IndexDataStore *store = static_cast<IndexDataStore*>(c_store);
  SmallVector<StringRef, 16> recNames;
  recNames.reserve(active_record_count);
  for (size_t i = 0; i != active_record_count; ++i) {
    recNames.push_back(StringRef(active_record_names[i].data, active_record_names[i].length));
  }
  store->purgeStaleRecords(recNames);
}

indexstore_symbol_kind_t
indexstore_symbol_get_kind(indexstore_symbol_t sym) {
  return getIndexStoreKind(static_cast<IndexRecordDecl *>(sym)->Kind);
}

indexstore_symbol_language_t
indexstore_symbol_get_language(indexstore_symbol_t sym) {
  return getIndexStoreLang(static_cast<IndexRecordDecl *>(sym)->Lang);
}

uint64_t
indexstore_symbol_get_sub_kinds(indexstore_symbol_t sym) {
  return getIndexStoreSubKinds(static_cast<IndexRecordDecl *>(sym)->SubKinds);
}

uint64_t
indexstore_symbol_get_roles(indexstore_symbol_t sym) {
  return getIndexStoreRoles(static_cast<IndexRecordDecl *>(sym)->Roles);
}

uint64_t
indexstore_symbol_get_related_roles(indexstore_symbol_t sym) {
  return getIndexStoreRoles(static_cast<IndexRecordDecl *>(sym)->RelatedRoles);
}

indexstore_string_ref_t
indexstore_symbol_get_name(indexstore_symbol_t sym) {
  auto *D = static_cast<IndexRecordDecl*>(sym);
  return toIndexStoreString(D->Name);
}

indexstore_string_ref_t
indexstore_symbol_get_usr(indexstore_symbol_t sym) {
  auto *D = static_cast<IndexRecordDecl*>(sym);
  return toIndexStoreString(D->USR);
}

indexstore_string_ref_t
indexstore_symbol_get_codegen_name(indexstore_symbol_t sym) {
  auto *D = static_cast<IndexRecordDecl*>(sym);
  return toIndexStoreString(D->CodeGenName);
}

uint64_t
indexstore_symbol_relation_get_roles(indexstore_symbol_relation_t sym_rel) {
  return getIndexStoreRoles(static_cast<IndexRecordRelation *>(sym_rel)->Roles);
}

indexstore_symbol_t
indexstore_symbol_relation_get_symbol(indexstore_symbol_relation_t sym_rel) {
  return (indexstore_symbol_t)static_cast<IndexRecordRelation*>(sym_rel)->Dcl;
}

indexstore_symbol_t
indexstore_occurrence_get_symbol(indexstore_occurrence_t occur) {
  return (indexstore_symbol_t)static_cast<IndexRecordOccurrence*>(occur)->Dcl;
}

#if INDEXSTORE_HAS_BLOCKS
bool
indexstore_occurrence_relations_apply(indexstore_occurrence_t occur,
                      bool(^applier)(indexstore_symbol_relation_t symbol_rel)) {
  auto *recOccur = static_cast<IndexRecordOccurrence*>(occur);
  for (auto &rel : recOccur->Relations) {
    if (!applier(&rel))
      return false;
  }
  return true;
}
#endif

uint64_t
indexstore_occurrence_get_roles(indexstore_occurrence_t occur) {
  return static_cast<IndexRecordOccurrence*>(occur)->Roles;
}

void
indexstore_occurrence_get_line_col(indexstore_occurrence_t occur,
                              unsigned *line, unsigned *column) {
  auto *recOccur = static_cast<IndexRecordOccurrence*>(occur);
  if (line)
    *line = recOccur->Line;
  if (column)
    *column = recOccur->Column;
}

typedef void *indexstore_record_reader_t;

indexstore_record_reader_t
indexstore_record_reader_create(indexstore_t c_store, const char *record_name,
                                indexstore_error_t *c_error) {
  IndexDataStore *store = static_cast<IndexDataStore*>(c_store);
  std::unique_ptr<IndexRecordReader> reader;
  std::string error;
  reader = IndexRecordReader::createWithRecordFilename(record_name,
                                                       store->getFilePath(),
                                                       error);
  if (!reader) {
    if (c_error)
      *c_error = new IndexStoreError{ error };
    return nullptr;
  }
  return reader.release();
}

void
indexstore_record_reader_dispose(indexstore_record_reader_t rdr) {
  auto *reader = static_cast<IndexRecordReader *>(rdr);
  delete reader;
}

#if INDEXSTORE_HAS_BLOCKS
/// Goes through the symbol data and passes symbols to \c receiver, for the
/// symbol data that \c filter returns true on.
///
/// This allows allocating memory only for the record symbols that the caller is
/// interested in.
bool
indexstore_record_reader_search_symbols(indexstore_record_reader_t rdr,
    bool(^filter)(indexstore_symbol_t symbol, bool *stop),
    void(^receiver)(indexstore_symbol_t symbol)) {
  auto *reader = static_cast<IndexRecordReader *>(rdr);

  auto filterFn = [&](const IndexRecordDecl &D) -> IndexRecordReader::DeclSearchReturn {
    bool stop = false;
    bool accept = filter((indexstore_symbol_t)&D, &stop);
    return { accept, !stop };
  };
  auto receiverFn = [&](const IndexRecordDecl *D) {
    receiver((indexstore_symbol_t)D);
  };

  return reader->searchDecls(filterFn, receiverFn);
}

bool
indexstore_record_reader_symbols_apply(indexstore_record_reader_t rdr,
                                        bool nocache,
                                   bool(^applier)(indexstore_symbol_t symbol)) {
  auto *reader = static_cast<IndexRecordReader *>(rdr);
  auto receiverFn = [&](const IndexRecordDecl *D) -> bool {
    return applier((indexstore_symbol_t)D);
  };
  return reader->foreachDecl(nocache, receiverFn);
}

bool
indexstore_record_reader_occurrences_apply(indexstore_record_reader_t rdr,
                                bool(^applier)(indexstore_occurrence_t occur)) {
  auto *reader = static_cast<IndexRecordReader *>(rdr);
  auto receiverFn = [&](const IndexRecordOccurrence &RO) -> bool {
    return applier((indexstore_occurrence_t)&RO);
  };
  return reader->foreachOccurrence(receiverFn);
}

/// \param symbols if non-zero \c symbols_count, indicates the list of symbols
/// that we want to get occurrences for. An empty array indicates that we want
/// occurrences for all symbols.
/// \param related_symbols Same as \c symbols but for related symbols.
bool
indexstore_record_reader_occurrences_of_symbols_apply(indexstore_record_reader_t rdr,
        indexstore_symbol_t *symbols, size_t symbols_count,
        indexstore_symbol_t *related_symbols, size_t related_symbols_count,
        bool(^applier)(indexstore_occurrence_t occur)) {
  auto *reader = static_cast<IndexRecordReader *>(rdr);
  auto receiverFn = [&](const IndexRecordOccurrence &RO) -> bool {
    return applier((indexstore_occurrence_t)&RO);
  };
  return reader->foreachOccurrence({(IndexRecordDecl**)symbols, symbols_count},
                                   {(IndexRecordDecl**)related_symbols, related_symbols_count},
                                   receiverFn);
}
#endif

size_t
indexstore_store_get_unit_name_from_output_path(indexstore_t store,
                                                const char *output_path,
                                                char *name_buf,
                                                size_t buf_size) {
  SmallString<256> unitName;
  IndexUnitWriter::getUnitNameForOutputFile(output_path, unitName);
  size_t nameLen = unitName.size();
  strlcpy(name_buf, unitName.c_str(), buf_size);
  return nameLen;
}

bool
indexstore_store_get_unit_modification_time(indexstore_t c_store,
                                            const char *unit_name,
                                            int64_t *seconds,
                                            int64_t *nanoseconds,
                                            indexstore_error_t *c_error) {
  IndexDataStore *store = static_cast<IndexDataStore*>(c_store);
  std::string error;
  auto optModTime = IndexUnitReader::getModificationTimeForUnit(unit_name,
                                              store->getFilePath(), error);
  if (!optModTime) {
    if (c_error)
      *c_error = new IndexStoreError{ error };
    return true;
  }

  if (seconds)
    *seconds = optModTime->seconds();
  if (nanoseconds)
    *nanoseconds = optModTime->nanoseconds();
  return false;
}

indexstore_unit_reader_t
indexstore_unit_reader_create(indexstore_t c_store, const char *unit_name,
                              indexstore_error_t *c_error) {
  IndexDataStore *store = static_cast<IndexDataStore*>(c_store);
  std::unique_ptr<IndexUnitReader> reader;
  std::string error;
  reader = IndexUnitReader::createWithUnitFilename(unit_name,
                                                   store->getFilePath(), error);
  if (!reader) {
    if (c_error)
      *c_error = new IndexStoreError{ error };
    return nullptr;
  }
  return reader.release();
}

void
indexstore_unit_reader_dispose(indexstore_unit_reader_t rdr) {
  auto reader = static_cast<IndexUnitReader*>(rdr);
  delete reader;
}

indexstore_string_ref_t
indexstore_unit_reader_get_provider_identifier(indexstore_unit_reader_t rdr) {
  auto reader = static_cast<IndexUnitReader*>(rdr);
  return toIndexStoreString(reader->getProviderIdentifier());
}

indexstore_string_ref_t
indexstore_unit_reader_get_provider_version(indexstore_unit_reader_t rdr) {
  auto reader = static_cast<IndexUnitReader*>(rdr);
  return toIndexStoreString(reader->getProviderVersion());
}

void
indexstore_unit_reader_get_modification_time(indexstore_unit_reader_t rdr,
                                             int64_t *seconds,
                                             int64_t *nanoseconds) {
  auto reader = static_cast<IndexUnitReader*>(rdr);
  sys::TimeValue timeVal = reader->getModificationTime();
  if (seconds)
    *seconds = timeVal.seconds();
  if (nanoseconds)
    *nanoseconds = timeVal.nanoseconds();
}

bool
indexstore_unit_reader_is_system_unit(indexstore_unit_reader_t rdr) {
  auto reader = static_cast<IndexUnitReader*>(rdr);
  return reader->isSystemUnit();
}

bool
indexstore_unit_reader_has_main_file(indexstore_unit_reader_t rdr) {
  auto reader = static_cast<IndexUnitReader*>(rdr);
  return reader->hasMainFile();
}

indexstore_string_ref_t
indexstore_unit_reader_get_main_file(indexstore_unit_reader_t rdr) {
  auto reader = static_cast<IndexUnitReader*>(rdr);
  return toIndexStoreString(reader->getMainFilePath());
}

indexstore_string_ref_t
indexstore_unit_reader_get_working_dir(indexstore_unit_reader_t rdr) {
  auto reader = static_cast<IndexUnitReader*>(rdr);
  return toIndexStoreString(reader->getWorkingDirectory());
}

indexstore_string_ref_t
indexstore_unit_reader_get_output_file(indexstore_unit_reader_t rdr) {
  auto reader = static_cast<IndexUnitReader*>(rdr);
  return toIndexStoreString(reader->getOutputFile());
}

indexstore_string_ref_t
indexstore_unit_reader_get_sysroot_path(indexstore_unit_reader_t rdr) {
  auto reader = static_cast<IndexUnitReader*>(rdr);
  return toIndexStoreString(reader->getSysrootPath());
}

indexstore_string_ref_t
indexstore_unit_reader_get_target(indexstore_unit_reader_t rdr) {
  auto reader = static_cast<IndexUnitReader*>(rdr);
  return toIndexStoreString(reader->getTarget());
}

indexstore_unit_dependency_kind_t
indexstore_unit_dependency_get_kind(indexstore_unit_dependency_t c_dep) {
  auto dep = static_cast<const IndexUnitReader::DependencyInfo*>(c_dep);
  switch (dep->Kind) {
  case IndexUnitReader::DependencyKind::Unit: return INDEXSTORE_UNIT_DEPENDENCY_UNIT;
  case IndexUnitReader::DependencyKind::Record: return INDEXSTORE_UNIT_DEPENDENCY_RECORD;
  case IndexUnitReader::DependencyKind::File: return INDEXSTORE_UNIT_DEPENDENCY_FILE;
  }
}

bool
indexstore_unit_dependency_is_system(indexstore_unit_dependency_t c_dep) {
  auto dep = static_cast<const IndexUnitReader::DependencyInfo*>(c_dep);
  return dep->IsSystem;
}

indexstore_string_ref_t
indexstore_unit_dependency_get_filepath(indexstore_unit_dependency_t c_dep) {
  auto dep = static_cast<const IndexUnitReader::DependencyInfo*>(c_dep);
  return toIndexStoreString(dep->FilePath);
}

indexstore_string_ref_t
indexstore_unit_dependency_get_name(indexstore_unit_dependency_t c_dep) {
  auto dep = static_cast<const IndexUnitReader::DependencyInfo*>(c_dep);
  return toIndexStoreString(dep->UnitOrRecordName);
}

#if INDEXSTORE_HAS_BLOCKS
bool
indexstore_unit_reader_dependencies_apply(indexstore_unit_reader_t rdr,
                             bool(^applier)(indexstore_unit_dependency_t)) {
  auto reader = static_cast<IndexUnitReader*>(rdr);
  return reader->foreachDependency([&](const IndexUnitReader::DependencyInfo &depInfo) -> bool {
    return applier((void*)&depInfo);
  });
}
#endif
