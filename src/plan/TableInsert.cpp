#include "catalog/CatCache.h"

#include "plan/TableInsert.h"
#include "execution/TableInsertState.h"
#include "index/Index.h"
#include "storage/Table.h"
#include "dbmain/Database.h"

namespace taco {

TableInsert::TableInsert(std::unique_ptr<PlanNode>&& child,
                         std::shared_ptr<const TableDesc> tabdesc)
: PlanNode(TAG(TableInsert), std::move(child))
, m_tabdesc(std::move(tabdesc))
{
    if (!Schema::Compatible(get_child(0)->get_output_schema(),
                            m_tabdesc->GetSchema())) {
        LOG(kError, "TableInsert: child schema is not compatible with the table");
    }
}

std::unique_ptr<TableInsert>
TableInsert::Create(std::unique_ptr<PlanNode>&& child,
                    std::shared_ptr<const TableDesc> tabdesc) {
    return absl::WrapUnique(new TableInsert(std::move(child), tabdesc));
}

TableInsert::~TableInsert() {}

void
TableInsert::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
}

std::unique_ptr<PlanExecState>
TableInsert::create_exec_state() const {
    std::unique_ptr<Table> table = Table::Create(m_tabdesc);
    std::vector<std::unique_ptr<Index>> idxs;
    Oid tabid = m_tabdesc->GetTableEntry()->tabid();
    for (Oid idxid : g_catcache->FindAllIndexesOfTable(tabid)) {
        idxs.emplace_back(Index::Create(g_catcache->FindIndexDesc(idxid)));
    }
    std::unique_ptr<PlanExecState> child = get_child(0)->create_exec_state();
    return absl::WrapUnique(new TableInsertState(
        this, std::move(table), std::move(idxs), std::move(child)));
}

const Schema*
TableInsert::get_output_schema() const {
    return g_actsch;
}

}
