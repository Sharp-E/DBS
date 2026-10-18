#include "plan/Selection.h"
#include "execution/SelectionState.h"

namespace taco {

Selection::Selection(std::unique_ptr<PlanNode>&& child,
                     std::unique_ptr<ExprNode>&& cond)
: PlanNode(TAG(Selection), std::move(child))
, m_cond(std::move(cond))
{
    if (!m_cond) {
        LOG(kError, "Selection requires a non-null condition");
    }
}

std::unique_ptr<Selection>
Selection::Create(std::unique_ptr<PlanNode>&& child,
                  std::unique_ptr<ExprNode>&& cond) {
    return absl::WrapUnique(new Selection(std::move(child), std::move(cond)));
}

Selection::~Selection() {
}

void
Selection::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
}

std::unique_ptr<PlanExecState>
Selection::create_exec_state() const {
    std::unique_ptr<PlanExecState> child = get_child(0)->create_exec_state();
    return absl::WrapUnique(new SelectionState(this, std::move(child)));
}

const Schema*
Selection::get_output_schema() const {
    return get_child(0)->get_output_schema();
}

}    // namespace taco
