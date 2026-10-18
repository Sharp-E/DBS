#include "execution/TempTableState.h"

namespace taco {

TempTableState::TempTableState(const TempTable* plan)
: PlanExecState(TAG(TempTableState)), m_plan(plan)
{
    // TODO: implement it
}

TempTableState::~TempTableState() {
    // TODO: implement it
}

void
TempTableState::node_properties_to_string(std::string& buf, int indent) const {
    // For debugging purpose, you can implement it base on your need.
}

void
TempTableState::init() {
    // TODO: implement it
}

bool
TempTableState::next_tuple() {
    // TODO: implement it
    return false;
}

std::vector<NullableDatumRef>
TempTableState::get_record() {
    // TODO: implement it
    return {};
}

void
TempTableState::close() {
    // TODO: implement it
}

void
TempTableState::rewind() {
    // TODO: implement it
}

Datum
TempTableState::save_position() const {
    // TODO: implement it
    return Datum::FromNull();
}

bool
TempTableState::rewind(DatumRef saved_position) {
    // TODO: implement it
    return false;
}

}
