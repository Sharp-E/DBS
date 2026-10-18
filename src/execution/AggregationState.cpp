#include "execution/AggregationState.h"
#include "utils/builtin_funcs.h"

namespace taco {

AggregationState::AggregationState(const Aggregation* plan,
                                   std::unique_ptr<PlanExecState>&& child)
: PlanExecState(TAG(AggregationState), std::move(child))
, m_plan(plan), m_computed(false), m_returned(false) {}

AggregationState::~AggregationState() {
    if (m_initialized) close();
}

void
AggregationState::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
}

void
AggregationState::init() {
    get_child(0)->init();
    m_result.clear();
    m_computed = false;
    m_returned = false;
    m_initialized = true;
}

void
AggregationState::Compute() {
    const size_t n = m_plan->m_exprs.size();
    std::vector<Datum> states;
    states.reserve(n);
    for (size_t i = 0; i < n; ++i)
        states.emplace_back(FunctionCall(m_plan->m_init[i]));

    PlanExecState* child = get_child(0);
    while (child->next_tuple()) {
        std::vector<NullableDatumRef> rec = child->get_record();
        for (size_t i = 0; i < n; ++i) {
            Datum val = m_plan->m_exprs[i]->Eval(rec);
            FunctionCall(m_plan->m_acc[i], states[i], val);
        }
    }

    m_result.clear();
    m_result.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        Datum res = FunctionCall(m_plan->m_fin[i], states[i]);
        if (res.HasExternalRef()) res = res.DeepCopy();
        m_result.emplace_back(std::move(res));
    }
    m_computed = true;
}

bool
AggregationState::next_tuple() {
    if (m_returned) return false;
    if (!m_computed) Compute();
    m_returned = true;
    return true;
}

std::vector<NullableDatumRef>
AggregationState::get_record() {
    std::vector<NullableDatumRef> out;
    out.reserve(m_result.size());
    for (const Datum& d : m_result) out.emplace_back(d);
    return out;
}

void
AggregationState::close() {
    m_result.clear();
    m_computed = false;
    m_returned = false;
    get_child(0)->close();
    m_initialized = false;
}

void
AggregationState::rewind() {
    m_returned = false;
}

Datum
AggregationState::save_position() const {
    LOG(kFatal, "not implemented (project 5)");
    return Datum::FromNull();
}

bool
AggregationState::rewind(DatumRef saved_position) {
    LOG(kFatal, "not implemented (project 5)");
    return false;
}

}    // namespace taco
