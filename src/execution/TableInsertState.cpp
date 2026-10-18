#include "execution/TableInsertState.h"
#include "storage/Table.h"

namespace taco {

TableInsertState::TableInsertState(const TableInsert* plan,
                                   std::unique_ptr<Table>&& table,
                                   std::vector<std::unique_ptr<Index>>&& idxs,
                                   std::unique_ptr<PlanExecState>&& child)
: PlanExecState(TAG(TableInsertState), std::move(child))
, m_plan(plan)
{
    // TODO: implement it
}

TableInsertState::~TableInsertState() {
    // TODO: implement it
}

void
TableInsertState::node_properties_to_string(std::string &buf, int indent) const {
    // For debugging purpose, you can implement it base on your need.
}

void
TableInsertState::init() {
    // TODO: implement it
}

bool
TableInsertState::next_tuple() {
    // TODO: implement it
    return false;
}

std::vector<NullableDatumRef>
TableInsertState::get_record() {
    // TODO: implement it
    return {};
}

void
TableInsertState::close() {
    // TODO: implement it
}

void
TableInsertState::rewind() {
    // TODO: implement it
}

}
