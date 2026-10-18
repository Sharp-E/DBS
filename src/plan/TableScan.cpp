#include "plan/TableScan.h"
#include "execution/TableScanState.h"

#include <absl/strings/str_format.h>

namespace taco {

TableScan::TableScan(std::shared_ptr<const TableDesc> tabdesc)
: PlanNode(TAG(TableScan))
{
    // TODO: implement it
}

std::unique_ptr<TableScan>
TableScan::Create(std::shared_ptr<const TableDesc> tabdesc) {
    ASSERT(tabdesc.get());
    return absl::WrapUnique(new TableScan(std::move(tabdesc)));
}

TableScan::~TableScan() {
    // TODO: implement it
}

void
TableScan::node_properties_to_string(std::string &buf, int indent) const {
    // For debugging purpose, you can implement it base on your need.
}

std::unique_ptr<PlanExecState>
TableScan::create_exec_state() const {
    // TODO: implement it
    return nullptr;
}

const Schema *
TableScan::get_output_schema() const {
    // TODO: implement it
    return nullptr;
}

}    // namespace taco
