#include "execution/ProjectionState.h"

namespace taco {

ProjectionState::ProjectionState(const Projection* plan,
                                 std::unique_ptr<PlanExecState>&& child)
: PlanExecState(TAG(ProjectionState), std::move(child))
, m_plan(plan)
{
}

ProjectionState::~ProjectionState() {
    if (m_initialized) {
        close();
    }
}

void
ProjectionState::init() {
    get_child(0)->init();
    m_cur.clear();
    m_initialized = true;
}

void
ProjectionState::ComputeCurrent() {
    std::vector<NullableDatumRef> rec = get_child(0)->get_record();
    m_cur.clear();
    m_cur.reserve(m_plan->m_exprs.size());
    for (const auto& expr : m_plan->m_exprs) {
        m_cur.emplace_back(expr->Eval(rec));
    }
}

bool
ProjectionState::next_tuple() {
    if (!get_child(0)->next_tuple()) {
        m_cur.clear();
        return false;
    }
    ComputeCurrent();
    return true;
}

void
ProjectionState::node_properties_to_string(std::string &buf, int indent) const {
    append_indent(buf, indent);
}

std::vector<NullableDatumRef>
ProjectionState::get_record() {
    std::vector<NullableDatumRef> out;
    out.reserve(m_cur.size());
    for (const Datum& d : m_cur) {
        out.emplace_back(d);
    }
    return out;
}

void
ProjectionState::close() {
    m_cur.clear();
    get_child(0)->close();
    m_initialized = false;
}

void
ProjectionState::rewind() {
    m_cur.clear();
    get_child(0)->rewind();
}

Datum
ProjectionState::save_position() const {
    return get_child(0)->save_position();
}

bool
ProjectionState::rewind(DatumRef saved_position) {
    bool ok = get_child(0)->rewind(saved_position);
    if (ok) {
        ComputeCurrent();
    } else {
        m_cur.clear();
    }
    return ok;
}

}
