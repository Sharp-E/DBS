#include "plan/TempTable.h"
#include "execution/TempTableState.h"

namespace taco {

TempTable::TempTable(const Schema* schema)
: PlanNode(TAG(TempTable))
, m_schema(schema)
{
    if (!schema) {
        LOG(kError, "TempTable requires a non-null schema");
    }
}

std::unique_ptr<TempTable>
TempTable::Create(const Schema* schema) {
    return absl::WrapUnique(new TempTable(schema));
}

TempTable::~TempTable() {
    // m_records owns all its Datums and frees them automatically
}

void
TempTable::insert_record(std::vector<Datum>&& record) {
    // make sure every stored Datum owns its memory
    for (auto& d : record) {
        if (d.HasExternalRef()) {
            d = d.DeepCopy();
        }
    }
    m_records.emplace_back(std::move(record));
}

void
TempTable::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
    absl::StrAppendFormat(&buf, "NUM_RECORDS = %d", (int) m_records.size());
}

std::unique_ptr<PlanExecState>
TempTable::create_exec_state() const {
    return absl::WrapUnique(new TempTableState(this));
}

const Schema*
TempTable::get_output_schema() const {
    return m_schema;
}

}
