#include "execution/TableScanState.h"

namespace taco {

// temporarily disable -Wunused-function
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

/*!
 * Encodes a record ID as a saved position in TableScanState.
 *
 * Feel free to devise your own encoding scheme. This function is only provided
 * as a hint of how to do so in Taco-DB.
 *
 * You might also want to decide how to denote the boundary cases (i.e., start
 * of scan and end of scan) in the saved positions. Hint: neither INVALID_PID
 * nor RESERVED_PID may appear in any valid record id.
 */
static Datum
RecordIdToTSSSavedPosition(RecordId recid) {
    // PageNumber and SlotId are both unsigned
    return Datum::From(
        (((uint64_t) recid.pid) << (sizeof(recid.sid) << 3)) |
        ((uint64_t) recid.sid));
}

static RecordId
TSSSavedPositionToRecordId(DatumRef d) {
    uint64_t v = d.GetUInt64();
    return RecordId(
        (PageNumber)((v >> (sizeof(SlotId) << 3)) & (~(PageNumber) 0)),
        (SlotId)(v & (~(SlotId) 0)));
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

TableScanState::TableScanState(const TableScan *plan,
                               std::unique_ptr<Table> table)
: PlanExecState(TAG(TableScanState))
, m_plan(plan)
, m_table(std::move(table))
{
}

TableScanState::~TableScanState() {
    if (m_initialized) {
        close();
    }
}

void
TableScanState::node_properties_to_string(std::string &buf, int indent) const {
    append_indent(buf, indent);
}

void
TableScanState::init() {
    m_iter.reset(new Table::Iterator(m_table->StartScan()));
    m_cur.clear();
    m_initialized = true;
}

bool
TableScanState::next_tuple() {
    m_cur.clear();
    if (!m_iter || !m_iter->Next()) {
        return false;
    }
    m_cur = m_plan->get_output_schema()->DissemblePayload(
        m_iter->GetCurrentRecord().GetData());
    return true;
}

std::vector<NullableDatumRef>
TableScanState::get_record() {
    std::vector<NullableDatumRef> out;
    out.reserve(m_cur.size());
    for (const Datum &d : m_cur) {
        out.emplace_back(d);
    }
    return out;
}

void
TableScanState::close() {
    m_cur.clear();
    if (m_iter) {
        m_iter->EndScan();
        m_iter.reset();
    }
    m_initialized = false;
}

void
TableScanState::rewind() {
    m_cur.clear();
    if (m_iter) {
        m_iter->EndScan();
    }
    m_iter.reset(new Table::Iterator(m_table->StartScan()));
}

Datum
TableScanState::save_position() const {
    LOG(kFatal, "TableScanState::save_position() not implemented (project 5)");
    return Datum::FromNull();
}

bool
TableScanState::rewind(DatumRef saved_position) {
    LOG(kFatal, "TableScanState::rewind(pos) not implemented (project 5)");
    return false;
}

}    // namespace taco
