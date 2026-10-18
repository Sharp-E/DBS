#include "execution/LimitState.h"

namespace taco {

LimitState::LimitState(const Limit* plan,
                       std::unique_ptr<PlanExecState>&& child)
: PlanExecState(TAG(LimitState), std::move(child))
, m_plan(plan)
, m_n(0)
{
}

LimitState::~LimitState() {
    if (m_initialized) {
        close();
    }
}

void
LimitState::init() {
    get_child(0)->init();
    m_n = 0;
    m_initialized = true;
}

bool
LimitState::next_tuple() {
    if (m_n >= m_plan->m_count) {
        return false;
    }
    if (!get_child(0)->next_tuple()) {
        // child exhausted; don't call it again
        m_n = m_plan->m_count;
        return false;
    }
    ++m_n;
    return true;
}

void
LimitState::node_properties_to_string(std::string &buf, int indent) const {
    append_indent(buf, indent);
    absl::StrAppendFormat(&buf, "N = %d", (int) m_n);
}

std::vector<NullableDatumRef>
LimitState::get_record() {
    return get_child(0)->get_record();
}

void
LimitState::close() {
    get_child(0)->close();
    m_n = 0;
    m_initialized = false;
}

void
LimitState::rewind() {
    get_child(0)->rewind();
    m_n = 0;
}

Datum
LimitState::save_position() const {
    LOG(kFatal, "LimitState::save_position() not implemented (project 5)");
    return Datum::FromNull();
}

bool
LimitState::rewind(DatumRef saved_position) {
    LOG(kFatal, "LimitState::rewind(pos) not implemented (project 5)");
    return false;
}

}    // namespace taco
