#include "execution/ProjectionState.h"

namespace taco {

ProjectionState::ProjectionState(const Projection* plan, std::unique_ptr<PlanExecState>&& child)
: PlanExecState(TAG(ProjectionState), std::move(child)), m_plan(plan)
{
    // TODO: implement it
}

ProjectionState::~ProjectionState() {
    // TODO: implement it
}

void
ProjectionState::init() {
    // TODO: implement it
}

bool
ProjectionState::next_tuple() {
    // TODO: implement it
    return false;
}

void
ProjectionState::node_properties_to_string(std::string& buf, int indent) const {
    // For debugging purpose, you can implement it base on your need.
}

std::vector<NullableDatumRef>
ProjectionState::get_record() {
    // TODO: implement it
    return {};
}

void
ProjectionState::close() {
    // TODO: implement it
}

void
ProjectionState::rewind() {
    // TODO: implement it
}

Datum
ProjectionState::save_position() const {
    // TODO: implement it
    return Datum::FromNull();
}

bool
ProjectionState::rewind(DatumRef saved_position) {
    // TODO: implement it
    return false;
}

}
