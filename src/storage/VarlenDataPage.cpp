#include "storage/VarlenDataPage.h"

#include "storage/FileManager.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace taco {

/*!
 * The page header of a variable-length record data page.
 *
 * Its layout is as follows:
 * |        header      | occupied space | free space | slot array |
 * | fixedhdr | usrdata | (maybe holes   ^            ^            ^
 *            ^         ^  in middle)    |            |            |
 *            |         |                |            |         PAGE_SIZE
 *    VarlenDataPageHeaderSize           |            |
 *                      |                |            |
 *                      |           m_fs_begin        |
 *                ph->m_ph_sz (maxaligned)            |
 *                                      PAGE_SIZE - sizeof(SlotData) * m_nslots
 *
 *
 * The slot array grows backwards (from high address to low). The slot for slot
 * `sid' can be indexed by `slot_array_end[-(int)sid]`, where `slot_array_end =
 * (SlotData*)(page_pointer + PAGE_SIZE)`.
 *
 */
struct VarlenDataPageHeader {
    //! The file manager managed page header. Do not modify.
    PageHeaderData  m_ph;

    //! The size of the header including the user data area.
    FieldOffset     m_ph_sz;

    //! The offset to the beginning of the free space.
    FieldOffset     m_fs_begin;

    /*!
     * Whether there might be some free space among the occupied space after
     * the last compaction. It can be an approximation but setting it to true
     * too aggresively may result in too many space compaction calls.
     */
    bool            m_has_hole : 1;

    // reserved
    bool            : 1;

    //! Number of items on this page.
    SlotId          m_cnt : 14;

    // reserved
    SlotId          :2;

    //! Number of slots in the slot array.
    SlotId          m_nslots : 14;
};

// max-aligned
static constexpr size_t VarlenDataPageHeaderSize =
    MAXALIGN(sizeof(VarlenDataPageHeader));

// we can manage to squeeze everything into 8 bytes after the page header.
static_assert(VarlenDataPageHeaderSize == MAXALIGN(8 + sizeof(PageHeaderData)));

/*!
 * Describes a slot. An invalid slot has an m_off == 0.
 */
struct SlotData {
    //! The offset to the record.
    FieldOffset     m_off;

    //! The length of the record.
    FieldOffset     m_len;

    constexpr bool
    IsValid() const {
        return m_off != 0;
    }
};

static inline VarlenDataPageHeader *
GetHdr(char *pagebuf) {
    return (VarlenDataPageHeader *) pagebuf;
}

static inline SlotData *
GetSlot(char *pagebuf, SlotId sid) {
    return &((SlotData *)(pagebuf + PAGE_SIZE))[-(int) sid];
}

static inline FieldOffset
RecordSpaceEnd(SlotId nslots) {
    size_t slot_begin = (size_t) PAGE_SIZE - sizeof(SlotData) * (size_t) nslots;
    return (FieldOffset)(slot_begin & ~((size_t) MAXALIGN_OF - 1));
}

static inline FieldOffset
AlignedLen(FieldOffset len) {
    return (FieldOffset) MAXALIGN(len);
}

static FieldOffset
UsedSpace(char *pagebuf, SlotId except) {
    VarlenDataPageHeader *hdr = GetHdr(pagebuf);
    FieldOffset total = 0;
    for (SlotId sid = MinSlotId; sid <= hdr->m_nslots; ++sid) {
        if (sid == except)
            continue;
        SlotData *slot = GetSlot(pagebuf, sid);
        if (slot->IsValid())
            total += AlignedLen(slot->m_len);
    }
    return total;
}

static void
CompactPage(char *pagebuf) {
    VarlenDataPageHeader *hdr = GetHdr(pagebuf);
    std::vector<SlotId> sids;
    for (SlotId sid = MinSlotId; sid <= hdr->m_nslots; ++sid) {
        if (GetSlot(pagebuf, sid)->IsValid())
            sids.push_back(sid);
    }
    std::sort(sids.begin(), sids.end(), [pagebuf](SlotId a, SlotId b) {
        return GetSlot(pagebuf, a)->m_off < GetSlot(pagebuf, b)->m_off;
    });

    FieldOffset off = hdr->m_ph_sz;
    for (SlotId sid : sids) {
        SlotData *slot = GetSlot(pagebuf, sid);
        if (slot->m_off != off) {
            memmove(pagebuf + off, pagebuf + slot->m_off, slot->m_len);
            slot->m_off = off;
        }
        off += AlignedLen(slot->m_len);
    }
    hdr->m_fs_begin = off;
    hdr->m_has_hole = false;
}

void
VarlenDataPage::InitializePage(char *pagebuf, FieldOffset usr_data_sz) {
    size_t ph_sz = MAXALIGN(VarlenDataPageHeaderSize + (size_t) usr_data_sz);
    if (ph_sz + MAXALIGN_OF > (size_t) RecordSpaceEnd(1)) {
        LOG(kError, "user data area of size %d is too large for a data page",
                    (int) usr_data_sz);
    }

    VarlenDataPageHeader *hdr = GetHdr(pagebuf);
    hdr->m_ph_sz = (FieldOffset) ph_sz;
    hdr->m_fs_begin = (FieldOffset) ph_sz;
    hdr->m_has_hole = false;
    hdr->m_cnt = 0;
    hdr->m_nslots = 0;
}

VarlenDataPage::VarlenDataPage(char *pagebuf):
    m_pagebuf(pagebuf) {
}

char*
VarlenDataPage::GetUserData() const {
    return m_pagebuf + VarlenDataPageHeaderSize;
}

SlotId
VarlenDataPage::GetMaxSlotId() const {
    return (SlotId) GetHdr(m_pagebuf)->m_nslots;
}

SlotId
VarlenDataPage::GetRecordCount() const {
    return (SlotId) GetHdr(m_pagebuf)->m_cnt;
}

char *
VarlenDataPage::GetRecordBuffer(SlotId sid, FieldOffset *p_len) const {
    CheckSID(sid);

    SlotData *slot = GetSlot(m_pagebuf, sid);
    if (p_len) {
        *p_len = slot->m_len;
    }
    return m_pagebuf + slot->m_off;
}

bool
VarlenDataPage::IsOccupied(SlotId sid) const {
    CheckSID(sid);

    return GetSlot(m_pagebuf, sid)->IsValid();
}

bool
VarlenDataPage::InsertRecord(Record &rec) {
    VarlenDataPageHeader *hdr = GetHdr(m_pagebuf);
    FieldOffset len = rec.GetLength();
    rec.GetRecordId().sid = INVALID_SID;

    FieldOffset max_len = RecordSpaceEnd(1) - hdr->m_ph_sz;
    if (len < 0 || len > max_len)
        return false;
    FieldOffset alen = AlignedLen(len);
    if (alen > max_len)
        return false;

    SlotId sid = INVALID_SID;
    for (SlotId i = MinSlotId; i <= hdr->m_nslots; ++i) {
        if (!GetSlot(m_pagebuf, i)->IsValid()) {
            sid = i;
            break;
        }
    }
    SlotId new_nslots = hdr->m_nslots;
    if (sid == INVALID_SID) {
        new_nslots = hdr->m_nslots + 1;
        sid = new_nslots;
    }
    FieldOffset end = RecordSpaceEnd(new_nslots);

    if (hdr->m_fs_begin + alen > end) {
        if (!hdr->m_has_hole ||
            hdr->m_ph_sz + UsedSpace(m_pagebuf, INVALID_SID) + alen > end) {
            rec.GetRecordId().sid = INVALID_SID;
            return true;
        }
        CompactPage(m_pagebuf);
    }

    if (sid > hdr->m_nslots) {
        hdr->m_nslots = sid;
    }
    SlotData *slot = GetSlot(m_pagebuf, sid);
    slot->m_off = hdr->m_fs_begin;
    slot->m_len = len;
    memcpy(m_pagebuf + slot->m_off, rec.GetData(), len);
    hdr->m_fs_begin = hdr->m_fs_begin + alen;
    hdr->m_cnt = hdr->m_cnt + 1;
    rec.GetRecordId().sid = sid;
    return true;
}

bool
VarlenDataPage::EraseRecord(SlotId sid) {
    CheckSID(sid);

    VarlenDataPageHeader *hdr = GetHdr(m_pagebuf);
    SlotData *slot = GetSlot(m_pagebuf, sid);
    if (!slot->IsValid())
        return false;

    FieldOffset alen = AlignedLen(slot->m_len);
    if (slot->m_off + alen == hdr->m_fs_begin) {
        hdr->m_fs_begin = slot->m_off;
    } else {
        hdr->m_has_hole = true;
    }
    slot->m_off = 0;
    slot->m_len = 0;
    hdr->m_cnt = hdr->m_cnt - 1;

    while (hdr->m_nslots > 0 &&
           !GetSlot(m_pagebuf, (SlotId) hdr->m_nslots)->IsValid()) {
        hdr->m_nslots = hdr->m_nslots - 1;
    }

    if (hdr->m_cnt == 0) {
        hdr->m_fs_begin = hdr->m_ph_sz;
        hdr->m_has_hole = false;
    }
    return true;
}

bool
VarlenDataPage::UpdateRecord(SlotId sid, Record &rec) {
    CheckSID(sid);
    if (!IsOccupied(sid)) {
        LOG(kError, "slot " SLOTID_FORMAT " is not occupied", sid);
    }

    VarlenDataPageHeader *hdr = GetHdr(m_pagebuf);
    SlotData *slot = GetSlot(m_pagebuf, sid);
    FieldOffset len = rec.GetLength();

    FieldOffset max_len = RecordSpaceEnd(1) - hdr->m_ph_sz;
    if (len < 0 || len > max_len)
        return false;
    FieldOffset alen = AlignedLen(len);
    if (alen > max_len)
        return false;

    FieldOffset old_alen = AlignedLen(slot->m_len);

    if (alen <= old_alen) {
        memmove(m_pagebuf + slot->m_off, rec.GetData(), len);
        if (alen < old_alen) {
            if (slot->m_off + old_alen == hdr->m_fs_begin) {
                hdr->m_fs_begin = slot->m_off + alen;
            } else {
                hdr->m_has_hole = true;
            }
        }
        slot->m_len = len;
        rec.GetRecordId().sid = sid;
        return true;
    }

    FieldOffset end = RecordSpaceEnd(hdr->m_nslots);
    if (slot->m_off + old_alen == hdr->m_fs_begin &&
        slot->m_off + alen <= end) {
        memmove(m_pagebuf + slot->m_off, rec.GetData(), len);
        hdr->m_fs_begin = slot->m_off + alen;
    } else if (hdr->m_fs_begin + alen <= end) {
        memcpy(m_pagebuf + hdr->m_fs_begin, rec.GetData(), len);
        slot->m_off = hdr->m_fs_begin;
        hdr->m_fs_begin = hdr->m_fs_begin + alen;
        hdr->m_has_hole = true;
    } else if (hdr->m_ph_sz + UsedSpace(m_pagebuf, sid) + alen <= end) {
        slot->m_off = 0;
        CompactPage(m_pagebuf);
        slot->m_off = hdr->m_fs_begin;
        memcpy(m_pagebuf + slot->m_off, rec.GetData(), len);
        hdr->m_fs_begin = hdr->m_fs_begin + alen;
    } else {
        EraseRecord(sid);
        rec.GetRecordId().sid = INVALID_SID;
        return true;
    }
    slot->m_len = len;
    rec.GetRecordId().sid = sid;
    return true;
}

bool
VarlenDataPage::InsertRecordAt(SlotId sid, Record &rec) {
    if (sid < GetMinSlotId() || sid > GetMaxSlotId() + 1) {
        LOG(kError, "sid is out of range, got " SLOTID_FORMAT " but "
                    "expecting in [" SLOTID_FORMAT ", " SLOTID_FORMAT "]",
                    sid, GetMinSlotId(), GetMaxSlotId() + 1);
    }

    // No need to implement it at this point in Project heap file
    return false;
}

void
VarlenDataPage::RemoveSlot(SlotId sid) {
    CheckSID(sid);

    // No need to implement it at this point in Project heap file
}

void
VarlenDataPage::ShiftSlots(SlotId n, bool truncate) {
    // No need to implement it at this point in Project heap file
    // Hint: don't forget to update m_has_hole in the page header
    //
    // This is part of the bonus project for b-tree page rebalancing. No need
    // to implement this function if you do not want to implement b-tree page
    // rebalancing.
}

FieldOffset
VarlenDataPage::ComputeFreeSpace(FieldOffset usr_data_sz,
                                 SlotId num_recs,
                                 FieldOffset total_reclen) {
    // No need to implement it at this point in Project heap file
    return -1;
}

}   // namespace taco
