#include "catalog/CatCache.h"

#include "plan/TableDelete.h"
#include "execution/TableDeleteState.h"
#include "storage/Table.h"
#include "index/Index.h"

namespace taco {

TableDelete::TableDelete(std::shared_ptr<const TableDesc> tabdesc,
                         std::unique_ptr<ExprNode>&& cond)
: PlanNode(TAG(TableDelete))
{
    // TODO: implement it
}

std::unique_ptr<TableDelete>
TableDelete::Create(std::shared_ptr<const TableDesc> tabdesc,
                    std::unique_ptr<ExprNode>&& cond) {
    return absl::WrapUnique(new TableDelete(tabdesc, std::move(cond)));
}

TableDelete::~TableDelete() {
    // TODO: implement it
}

void
TableDelete::node_properties_to_string(std::string& buf, int indent) const {
    // For debugging purpose, you can implement it base on your need.
}

std::unique_ptr<PlanExecState>
TableDelete::create_exec_state() const {
    // TODO: implement it
    return nullptr;
}

const Schema*
TableDelete::get_output_schema() const {
    // TODO: implement it
    return nullptr;
}

}
