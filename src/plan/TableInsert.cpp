#include "catalog/CatCache.h"

#include "plan/TableInsert.h"
#include "execution/TableInsertState.h"
#include "index/Index.h"
#include "storage/Table.h"

namespace taco {

TableInsert::TableInsert(std::unique_ptr<PlanNode>&& child,
                         std::shared_ptr<const TableDesc> tabdesc)
: PlanNode(TAG(TableInsert), std::move(child))
{
    // TODO: implement it
}

std::unique_ptr<TableInsert>
TableInsert::Create(std::unique_ptr<PlanNode>&& child,
                    std::shared_ptr<const TableDesc> tabdesc) {
    return absl::WrapUnique(new TableInsert(std::move(child), tabdesc));
}

TableInsert::~TableInsert() {
    // TODO: implement it
}

void
TableInsert::node_properties_to_string(std::string& buf, int indent) const {
    // For debugging purpose, you can implement it base on your need.
}

std::unique_ptr<PlanExecState>
TableInsert::create_exec_state() const {
    // TODO: implement it
    return nullptr;
}

const Schema*
TableInsert::get_output_schema() const {
    // TODO: implement it
    return nullptr;
}

}
