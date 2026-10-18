#include "execution/TableInsertState.h"
#include "storage/Table.h"

namespace taco {

TableInsertState::TableInsertState(const TableInsert* plan,
                                   std::unique_ptr<Table>&& table,
                                   std::vector<std::unique_ptr<Index>>&& idxs,
                                   std::unique_ptr<PlanExecState>&& child)
: PlanExecState(TAG(TableInsertState), std::move(child))
, m_plan(plan), m_table(std::move(table)), m_idxs(std::move(idxs))
, m_done(false) {}

TableInsertState::~TableInsertState() {
    if (m_initialized) close();
}

void
TableInsertState::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
}

void
TableInsertState::init() {
    get_child(0)->init();
    m_done = false;
    m_initialized = true;
}

bool
TableInsertState::next_tuple() {
    if (m_done) return false;

    const Schema* tabsch = m_plan->m_tabdesc->GetSchema();
    PlanExecState* child = get_child(0);
    maxaligned_char_buf buf;
    int64_t cnt = 0;
    while (child->next_tuple()) {
        std::vector<NullableDatumRef> rec = child->get_record();
        buf.clear();
        FieldOffset len = tabsch->WritePayloadToBuffer(rec, buf);
        Record r(buf.data(), len);
        m_table->InsertRecord(r);
        RecordId rid = r.GetRecordId();

        for (auto& idx : m_idxs) {
            const IndexDesc* idxdesc = idx->GetIndexDesc();
            const Schema* keysch = idxdesc->GetKeySchema();
            std::vector<NullableDatumRef> keydata;
            for (FieldId i = 0; i < keysch->GetNumFields(); ++i) {
                FieldId tabcol = idxdesc->GetIndexColumnEntry(i)->idxcoltabcolid();
                keydata.push_back(rec[tabcol]);
            }
            auto key = IndexKey::Create(keydata);
            idx->InsertKey(key.get(), rid);
        }
        ++cnt;
    }
    m_result = Datum::From(cnt);
    m_done = true;
    return true;
}

std::vector<NullableDatumRef>
TableInsertState::get_record() {
    return {NullableDatumRef(m_result)};
}

void
TableInsertState::close() {
    get_child(0)->close();
    m_initialized = false;
}

void
TableInsertState::rewind() {
    LOG(kError, "TableInsertState cannot be rewound");
}

}
