#include "execution/SelectionState.h"

namespace taco {

SelectionState::SelectionState(const Selection* plan,
                               std::unique_ptr<PlanExecState>&& child)
: PlanExecState(TAG(SelectionState), std::move(child))
, m_plan(plan)
{
}

SelectionState::~SelectionState() {
    if (m_initialized) {
        close();
    }
}

void
SelectionState::init() {
    get_child(0)->init();
    m_cur.clear();
    m_initialized = true;
}

bool
SelectionState::next_tuple() {
    PlanExecState* child = get_child(0);
    while (child->next_tuple()) {
        std::vector<NullableDatumRef> rec = child->get_record();
        Datum res = m_plan->m_cond->Eval(rec);
        if (!res.isnull() && res.GetBool()) {
            m_cur = std::move(rec);
            return true;
        }
    }
    m_cur.clear();
    return false;
}

void
SelectionState::node_properties_to_string(std::string &buf, int indent) const {
    append_indent(buf, indent);
}

std::vector<NullableDatumRef>
SelectionState::get_record() {
    return m_cur;
}

void
SelectionState::close() {
    m_cur.clear();
    get_child(0)->close();
    m_initialized = false;
}

void
SelectionState::rewind() {
    m_cur.clear();
    get_child(0)->rewind();
}

Datum
SelectionState::save_position() const {
    return get_child(0)->save_position();
}

bool
SelectionState::rewind(DatumRef saved_position) {
    bool ok = get_child(0)->rewind(saved_position);
    if (ok) {
        m_cur = get_child(0)->get_record();
    } else {
        m_cur.clear();
    }
    return ok;
}

}    // namespace taco
