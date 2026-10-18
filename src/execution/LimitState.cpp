#include "execution/LimitState.h"

namespace taco {

LimitState::LimitState(const Limit* plan, std::unique_ptr<PlanExecState>&& child)
: PlanExecState(TAG(LimitState), std::move(child)), m_plan(plan)
{
    // TODO: implement it
}

LimitState::~LimitState() {
    // TODO: implement it
}

void
LimitState::init() {
    // TODO: implement it
}

bool
LimitState::next_tuple() {
    // TODO: implement it
    return false;
}

void
LimitState::node_properties_to_string(std::string& buf, int indent) const {
    // For debugging purpose, you can implement it base on your need.
}

std::vector<NullableDatumRef>
LimitState::get_record() {
    // TODO: implement it
    return {};
}


void
LimitState::close() {
    // TODO: implement it
}

void
LimitState::rewind() {
    // TODO: implement it
}

Datum
LimitState::save_position() const {
    // TODO: implement it
    return Datum::FromNull();
}

bool
LimitState::rewind(DatumRef saved_position) {
    // TODO: implement it
    return false;
}


}
