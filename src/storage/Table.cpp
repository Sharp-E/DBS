#include "storage/Table.h"

#include "dbmain/Database.h"
#include "storage/BufferManager.h"
#include "storage/FileManager.h"
#include "storage/VarlenDataPage.h"
#include "utils/Latch.h"

namespace taco {

void
Table::Initialize(const TableDesc *tabdesc) {
    std::unique_ptr<File> f = g_fileman->Open(tabdesc->GetTableEntry()->tabfid());
    char *buf;
    ScopedBufferId bid = g_bufman->PinPage(f->GetFirstPageNumber(), &buf);
    VarlenDataPage::InitializePage(buf);
    g_bufman->MarkDirty(bid);
}

std::unique_ptr<Table>
Table::Create(std::shared_ptr<const TableDesc> tabdesc) {
    std::unique_ptr<File> f = g_fileman->Open(tabdesc->GetTableEntry()->tabfid());
    return absl::WrapUnique(new Table(std::move(tabdesc), std::move(f)));
}

Table::Table(std::shared_ptr<const TableDesc> tabdesc,
             std::unique_ptr<File> file):
    m_tabdesc(std::move(tabdesc)),
    m_file(std::move(file)) {
}

Table::~Table() {
}

void
Table::InsertRecord(Record& rec) {
    PageNumber pid = m_file->GetLastPageNumber();
    {
        char *buf;
        ScopedBufferId bid = g_bufman->PinPage(pid, &buf);
        VarlenDataPage dp(buf);
        if (!dp.InsertRecord(rec)) {
            LOG(kError, "record of length %d is too long to be inserted",
                        (int) rec.GetLength());
        }
        if (rec.GetRecordId().sid != INVALID_SID) {
            g_bufman->MarkDirty(bid);
            rec.GetRecordId().pid = pid;
            return;
        }
    }

    ScopedBufferId bid = m_file->AllocatePage(LatchMode::EX);
    char *buf = g_bufman->GetBuffer(bid);
    VarlenDataPage::InitializePage(buf);
    VarlenDataPage dp(buf);
    dp.InsertRecord(rec);
    g_bufman->MarkDirty(bid);
    if (rec.GetRecordId().sid == INVALID_SID) {
        LOG(kError, "unable to insert record of length %d into an empty page",
                    (int) rec.GetLength());
    }
    rec.GetRecordId().pid = m_file->GetLastPageNumber();
}

void
Table::EraseRecord(RecordId rid) {
    if (rid.pid == INVALID_PID) {
        LOG(kError, "invalid record id");
    }
    char *buf;
    ScopedBufferId bid = g_bufman->PinPage(rid.pid, &buf, m_file->GetFileId());
    VarlenDataPage dp(buf);
    if (!dp.EraseRecord(rid.sid)) {
        LOG(kError, "record (%u, %u) not found",
                    (unsigned) rid.pid, (unsigned) rid.sid);
    }
    g_bufman->MarkDirty(bid);
    if (dp.GetRecordCount() == 0 && rid.pid != m_file->GetFirstPageNumber()) {
        m_file->FreePage(bid);
    }
}

void
Table::UpdateRecord(RecordId rid, Record &rec) {
    if (rid.pid == INVALID_PID) {
        LOG(kError, "invalid record id");
    }
    char *buf;
    ScopedBufferId bid = g_bufman->PinPage(rid.pid, &buf, m_file->GetFileId());
    VarlenDataPage dp(buf);
    if (!dp.IsOccupied(rid.sid)) {
        LOG(kError, "record (%u, %u) not found",
                    (unsigned) rid.pid, (unsigned) rid.sid);
    }
    if (!dp.UpdateRecord(rid.sid, rec)) {
        LOG(kError, "record of length %d is too long for update",
                    (int) rec.GetLength());
    }
    g_bufman->MarkDirty(bid);
    if (rec.GetRecordId().sid != INVALID_SID) {
        rec.GetRecordId().pid = rid.pid;
        return;
    }

    InsertRecord(rec);
    if (dp.GetRecordCount() == 0 && rid.pid != m_file->GetFirstPageNumber()) {
        m_file->FreePage(bid);
    }
}

Table::Iterator
Table::StartScan() {
    return Iterator(this, RecordId(m_file->GetFirstPageNumber(), MinSlotId));
}

Table::Iterator
Table::StartScanFrom(RecordId rid) {
    if (rid.pid == INVALID_PID) {
        return StartScan();
    }
    return Iterator(this, rid);
}

Table::Iterator::Iterator(Table* tbl, RecordId rid):
    m_table(tbl),
    m_cur_record(),
    m_pinned_bufid(),
    m_pid(rid.pid),
    m_sid(rid.sid < MinSlotId ? MinSlotId : rid.sid) {
}

bool
Table::Iterator::Next() {
    if (!m_table) {
        return false;
    }
    m_cur_record.Clear();
    while (m_pid != INVALID_PID) {
        char *buf;
        if (m_pinned_bufid.IsValid()) {
            buf = g_bufman->GetBuffer(m_pinned_bufid);
        } else {
            m_pinned_bufid = g_bufman->PinPage(m_pid, &buf);
        }
        VarlenDataPage dp(buf);
        SlotId maxsid = dp.GetMaxSlotId();
        for (SlotId sid = m_sid; sid <= maxsid; ++sid) {
            if (dp.IsOccupied(sid)) {
                m_cur_record = dp.GetRecord(sid);
                m_cur_record.GetRecordId().pid = m_pid;
                m_sid = sid + 1;
                return true;
            }
        }
        PageNumber next_pid = ((PageHeaderData *) buf)->GetNextPageNumber();
        m_pinned_bufid.Reset();
        m_pid = next_pid;
        m_sid = MinSlotId;
    }
    return false;
}

void
Table::Iterator::EndScan() {
    m_cur_record.Clear();
    m_pinned_bufid.Reset();
    m_table = nullptr;
    m_pid = INVALID_PID;
    m_sid = INVALID_SID;
}

}   // namespace taco
