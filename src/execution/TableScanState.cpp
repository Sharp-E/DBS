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
{
    // TODO: implement it
}

TableScanState::~TableScanState() {
    // TODO: implement it
}

void
TableScanState::node_properties_to_string(std::string &buf, int indent) const {
    // For debugging purpose, you can implement it base on your need.
}

void
TableScanState::init() {
    // TODO: implement it
}

bool
TableScanState::next_tuple() {
    // TODO: implement it
    return false;
}

std::vector<NullableDatumRef>
TableScanState::get_record() {
    // TODO: implement it
    return {};
}

void
TableScanState::close() {
    // TODO: implement it
}

void
TableScanState::rewind() {
    // TODO: implement it
}

Datum
TableScanState::save_position() const {
    // TODO: implement it
    return Datum::FromNull();
}

bool
TableScanState::rewind(DatumRef saved_position) {
    // TODO: implement it
    return false;
}

}    // namespace taco
