#include "plan/Limit.h"
#include "execution/LimitState.h"

namespace taco {

Limit::Limit(std::unique_ptr<PlanNode>&& child, size_t count)
: PlanNode(TAG(Limit), std::move(child))
, m_count(count)
{
}

std::unique_ptr<Limit>
Limit::Create(std::unique_ptr<PlanNode>&& child, size_t count) {
    return absl::WrapUnique(new Limit(std::move(child), count));
}

Limit::~Limit() {
}

void
Limit::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
    absl::StrAppendFormat(&buf, "COUNT = %d", (int) m_count);
}

std::unique_ptr<PlanExecState>
Limit::create_exec_state() const {
    std::unique_ptr<PlanExecState> child = get_child(0)->create_exec_state();
    return absl::WrapUnique(new LimitState(this, std::move(child)));
}

const Schema*
Limit::get_output_schema() const {
    return get_child(0)->get_output_schema();
}

}    // namespace taco
