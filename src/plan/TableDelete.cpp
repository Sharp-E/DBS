#include "catalog/CatCache.h"

#include "plan/TableDelete.h"
#include "execution/TableDeleteState.h"
#include "storage/Table.h"
#include "index/Index.h"
#include "dbmain/Database.h"

namespace taco {

TableDelete::TableDelete(std::shared_ptr<const TableDesc> tabdesc,
                         std::unique_ptr<ExprNode>&& cond)
: PlanNode(TAG(TableDelete))
, m_tabdesc(std::move(tabdesc))
, m_cond(std::move(cond)) {}

std::unique_ptr<TableDelete>
TableDelete::Create(std::shared_ptr<const TableDesc> tabdesc,
                    std::unique_ptr<ExprNode>&& cond) {
    return absl::WrapUnique(new TableDelete(tabdesc, std::move(cond)));
}

TableDelete::~TableDelete() {}

void
TableDelete::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
}

std::unique_ptr<PlanExecState>
TableDelete::create_exec_state() const {
    std::unique_ptr<Table> table = Table::Create(m_tabdesc);
    std::vector<std::unique_ptr<Index>> idxs;
    Oid tabid = m_tabdesc->GetTableEntry()->tabid();
    for (Oid idxid : g_catcache->FindAllIndexesOfTable(tabid)) {
        idxs.emplace_back(Index::Create(g_catcache->FindIndexDesc(idxid)));
    }
    return absl::WrapUnique(new TableDeleteState(
        this, std::move(table), std::move(idxs)));
}

const Schema*
TableDelete::get_output_schema() const {
    return g_actsch;
}

}
