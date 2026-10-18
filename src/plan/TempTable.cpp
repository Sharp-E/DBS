#include "plan/TempTable.h"
#include "execution/TempTableState.h"

namespace taco {

TempTable::TempTable(const Schema* schema)
: PlanNode(TAG(TempTable))
{
    // TODO: implement it
}

std::unique_ptr<TempTable>
TempTable::Create(const Schema* schema) {
    return absl::WrapUnique(new TempTable(schema));
}

TempTable::~TempTable() {
    // TODO: implement it
}

void
TempTable::insert_record(std::vector<Datum>&& record) {
    // TODO: implement it
}

void
TempTable::node_properties_to_string(std::string& buf, int indent) const {
    // For debugging purpose, you can implement it base on your need.
}

std::unique_ptr<PlanExecState>
TempTable::create_exec_state() const {
    // TODO: implement it
    return nullptr;
}

const Schema*
TempTable::get_output_schema() const {
    // TODO: implement it
    return nullptr;
}

}
