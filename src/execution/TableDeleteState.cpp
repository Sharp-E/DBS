#include "execution/TableDeleteState.h"
#include "storage/Table.h"

namespace taco {

TableDeleteState::TableDeleteState(const TableDelete* plan,
                                   std::unique_ptr<Table>&& table,
                                   std::vector<std::unique_ptr<Index>>&& idxs)
: PlanExecState(TAG(TableDeleteState))
, m_plan(plan), m_table(std::move(table)), m_idxs(std::move(idxs))
, m_done(false) {}

TableDeleteState::~TableDeleteState() {
    if (m_initialized) close();
}

void
TableDeleteState::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
}

void
TableDeleteState::init() {
    m_done = false;
    m_initialized = true;
}

bool
TableDeleteState::next_tuple() {
    if (m_done) return false;

    const Schema* tabsch = m_plan->m_tabdesc->GetSchema();
    std::vector<RecordId> to_erase;

    Table::Iterator it = m_table->StartScan();
    while (it.Next()) {
        const char* data = it.GetCurrentRecord().GetData();
        if (m_plan->m_cond) {
            Datum c = m_plan->m_cond->Eval(data);
            if (c.isnull() || !c.GetBool()) continue;
        }
        RecordId rid = it.GetCurrentRecordId();

        if (!m_idxs.empty()) {
            std::vector<Datum> fields = tabsch->DissemblePayload(data);
            for (auto& idx : m_idxs) {
                const IndexDesc* idxdesc = idx->GetIndexDesc();
                const Schema* keysch = idxdesc->GetKeySchema();
                std::vector<NullableDatumRef> keydata;
                for (FieldId i = 0; i < keysch->GetNumFields(); ++i) {
                    FieldId tabcol = idxdesc->GetIndexColumnEntry(i)->idxcoltabcolid();
                    keydata.emplace_back(fields[tabcol]);
                }
                auto key = IndexKey::Create(keydata);
                RecordId r = rid;
                idx->DeleteKey(key.get(), r);
            }
        }
        to_erase.push_back(rid);
    }
    it.EndScan();

    // erase after the scan so no page is freed while pinned by the iterator
    for (const RecordId& rid : to_erase) {
        m_table->EraseRecord(rid);
    }
    m_result = Datum::From((int64_t) to_erase.size());
    m_done = true;
    return true;
}

std::vector<NullableDatumRef>
TableDeleteState::get_record() {
    return {NullableDatumRef(m_result)};
}

void
TableDeleteState::close() {
    m_initialized = false;
}

void
TableDeleteState::rewind() {
    LOG(kError, "TableDeleteState cannot be rewound");
}

}
