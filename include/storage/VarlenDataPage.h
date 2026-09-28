#ifndef STORAGE_VARLENDATAPAGE_H
#define STORAGE_VARLENDATAPAGE_H

#include "tdb.h"

#include "storage/Record.h"

namespace taco {

struct SlotData;

static_assert(MinSlotId == 1);

class VarlenDataPage {
public:

    static void InitializePage(char *pagebuf, FieldOffset usr_data_sz = 0);

    VarlenDataPage(char *pagebuf);

    char* GetUserData() const;

    SlotId
    GetMinSlotId() const {
        return MinSlotId;
    }

    SlotId GetMaxSlotId() const;

    SlotId
    GetMinOccupiedSlotId() const {
        for (SlotId sid = GetMinSlotId(); sid <= GetMaxSlotId(); ++sid) {
            if (IsOccupied(sid)) {
                return sid;
            }
        }
        return INVALID_SID;
    }

    SlotId
    GetMaxOccupiedSlotId() const {
        return GetMaxSlotId();
    }

    bool IsOccupied(SlotId sid) const;

    SlotId GetRecordCount() const;

    char *GetRecordBuffer(SlotId sid, FieldOffset *p_len) const;

    Record
    GetRecord(SlotId sid) const {
        Record rec;
        rec.GetData() = GetRecordBuffer(sid, &rec.GetLength());
        rec.GetRecordId().sid = sid;
        return rec;
    }

    bool InsertRecord(Record &rec);

    bool EraseRecord(SlotId sid);

    bool UpdateRecord(SlotId sid, Record &rec);

    bool InsertRecordAt(SlotId sid, Record &rec);

    void RemoveSlot(SlotId sid);

    void ShiftSlots(SlotId n, bool truncate);

    static FieldOffset ComputeFreeSpace(FieldOffset usr_data_sz,
                                        SlotId num_recs,
                                        FieldOffset total_reclen);
private:

    inline void
    CheckSID(SlotId sid) const {
        if (sid < GetMinSlotId() || sid > GetMaxSlotId()) {
            LOG(kError, "sid is out of range, got " SLOTID_FORMAT " but "
                        "expecting in [" SLOTID_FORMAT ", " SLOTID_FORMAT "]",
                        sid, GetMinSlotId(), GetMaxSlotId());
        }
    }

    char *m_pagebuf;
};

}

#endif
