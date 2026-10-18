#include "plan/Projection.h"
#include "execution/ProjectionState.h"

namespace taco {

Projection::Projection(std::unique_ptr<PlanNode>&& child,
                       std::vector<std::unique_ptr<ExprNode>>&& exprs)
: PlanNode(TAG(Projection), std::move(child))
{
    // TODO: implement it
}

std::unique_ptr<Projection>
Projection::Create(std::unique_ptr<PlanNode>&& child,
                   std::vector<std::unique_ptr<ExprNode>>&& exprs) {

    return absl::WrapUnique(new Projection(std::move(child), std::move(exprs)));
}

Projection::~Projection() {
    // TODO: implement it
}

void
Projection::node_properties_to_string(std::string& buf, int indent) const {
    // For debugging purpose, you can implement it base on your need.
}

std::unique_ptr<PlanExecState> Projection::create_exec_state() const {
    // TODO: implement it
    return nullptr;
}

const Schema*
Projection::get_output_schema() const {
    // TODO: implement it
    return nullptr;
}



}
