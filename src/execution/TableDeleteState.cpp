#include "execution/TableDeleteState.h"
#include "storage/Table.h"
#include "storage/VarlenDataPage.h"

namespace taco {

TableDeleteState::TableDeleteState(const TableDelete* plan,
                                   std::unique_ptr<Table>&& table,
                                   std::vector<std::unique_ptr<Index>>&& idxs)
: PlanExecState(TAG(TableDeleteState))
, m_plan(plan)
{
    // TODO: implement it
}

TableDeleteState::~TableDeleteState() {
    // TODO: implement it
}

void
TableDeleteState::node_properties_to_string(std::string& buf, int indent) const {
    // For debugging purpose, you can implement it base on your need.
}

void
TableDeleteState::init() {
    // TODO: implement it
}

bool
TableDeleteState::next_tuple() {
    // TODO: implement it
    return false;
}

std::vector<NullableDatumRef>
TableDeleteState::get_record() {
    // TODO: implement it
    return {};
}

void
TableDeleteState::close() {
    // TODO: implement it
}

void
TableDeleteState::rewind() {
    // TODO: implement it
}

}
