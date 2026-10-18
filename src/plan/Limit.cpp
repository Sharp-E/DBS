#include "plan/Limit.h"
#include "execution/LimitState.h"
#include "catalog/systables/initoids.h"

#include <absl/strings/str_format.h>

namespace taco {

Limit::Limit(std::unique_ptr<PlanNode>&& child, size_t cnt)
: PlanNode(TAG(Limit), std::move(child))
{
    // TODO: implement it
}

std::unique_ptr<Limit>
Limit::Create(std::unique_ptr<PlanNode>&& child, size_t cnt) {
    return absl::WrapUnique(new Limit(std::move(child), cnt));
}

Limit::~Limit() {
    // TODO: implement it
}

void
Limit::node_properties_to_string(std::string& buf, int indent) const {
    // For debugging purpose, you can implement it base on your need.
}

std::unique_ptr<PlanExecState>
Limit::create_exec_state() const {
    // TODO: implement it
    return nullptr;
}

const Schema*
Limit::get_output_schema() const {
    // TODO: implement it
    return nullptr;
}


}
