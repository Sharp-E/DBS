#pragma once

#include "tdb.h"

#include <unordered_set>

#include "catalog/TableDesc.h"
#include "storage/BufferManager.h"
#include "storage/FileManager.h"
#include "storage/Record.h"

namespace taco {

class File;

class Table {
public:
    class Iterator;

    static void Initialize(const TableDesc *tabdesc);

    static std::unique_ptr<Table> Create(
        std::shared_ptr<const TableDesc> tabdesc);

private:

    Table(std::shared_ptr<const TableDesc> tabdesc,
          std::unique_ptr<File> file);

public:

    ~Table();

    const TableDesc*
    GetTableDesc() const {
        return m_tabdesc.get();
    }

    void InsertRecord(Record& rec);

    void EraseRecord(RecordId rid);

    void UpdateRecord(RecordId rid, Record &rec);

    Iterator StartScan();

    Iterator StartScanFrom(RecordId rid);

private:

    std::shared_ptr<const TableDesc> m_tabdesc;

    std::unique_ptr<File> m_file;

public:

    class Iterator {
    public:

        Iterator():
            m_table(nullptr) {}

    private:

        Iterator(Table* tbl, RecordId rid);

    public:

        ~Iterator() {
            try {
                if (GetTable()) {
                    EndScan();
                }
            } catch (const TDBError &e) {
                LOG(kWarning, "unable to destruct the iterator due to an error "
                        "in Table::Iterator::EndScan(): \n%s", e.GetMessage());
            }
        }

        Iterator(Iterator&& it) = default;

        Iterator &operator=(Iterator &&it) = default;

        Iterator(const Iterator& it) = delete;

        Iterator &operator=(const Iterator &it) = delete;

        Table*
        GetTable() const {
            return m_table;
        }

        const Record&
        GetCurrentRecord() const {
            return m_cur_record;
        }

        RecordId
        GetCurrentRecordId() const {
            return m_cur_record.GetRecordId();
        }

        bool
        IsAtValidRecord() const {
            return m_cur_record.IsValid();
        }

        bool Next();

        void EndScan();

    private:
        Table*          m_table;

        Record          m_cur_record;

        ScopedBufferId  m_pinned_bufid;

        PageNumber      m_pid = INVALID_PID;

        SlotId          m_sid = INVALID_SID;

        friend class Table;
    };

    friend class Iterator;
};

}
