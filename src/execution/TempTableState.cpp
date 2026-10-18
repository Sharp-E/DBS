#include "execution/TempTableState.h"

namespace taco {

TempTableState::TempTableState(const TempTable* plan)
: PlanExecState(TAG(TempTableState)), m_plan(plan), m_pos(0)
{
}

TempTableState::~TempTableState() {
    if (m_initialized) {
        close();
    }
}

void
TempTableState::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
    absl::StrAppendFormat(&buf, "POS = %d", (int) m_pos);
}

void
TempTableState::init() {
    m_pos = 0;
    m_initialized = true;
}

bool
TempTableState::next_tuple() {
    if (m_pos < (uint64_t) m_plan->m_records.size()) {
        ++m_pos;
        return true;
    }
    // stay one past the end so repeated calls keep returning false
    m_pos = (uint64_t) m_plan->m_records.size() + 1;
    return false;
}

std::vector<NullableDatumRef>
TempTableState::get_record() {
    // references point into the plan's Datums, which outlive this state
    const std::vector<Datum>& rec = m_plan->m_records[m_pos - 1];
    std::vector<NullableDatumRef> out;
    out.reserve(rec.size());
    for (const Datum& d : rec) {
        out.emplace_back(d);
    }
    return out;
}

void
TempTableState::close() {
    m_pos = 0;
    m_initialized = false;
}

void
TempTableState::rewind() {
    m_pos = 0;
}

Datum
TempTableState::save_position() const {
    return Datum::From(m_pos);
}

bool
TempTableState::rewind(DatumRef saved_position) {
    m_pos = saved_position.GetUInt64();
    return m_pos >= 1 && m_pos <= (uint64_t) m_plan->m_records.size();
}

}
