#include "execution/SortState.h"

#include <absl/flags/flag.h>

ABSL_FLAG(size_t, sort_nways, 8,
          "performs n-way external sorting in the sort operator");

namespace taco {

namespace {

// Feeds the child's records into ExternalSort as serialized payloads.
class ChildItemIterator: public ItemIterator {
public:
    ChildItemIterator(PlanExecState* child, const Schema* sch)
    : m_child(child), m_sch(sch), m_len(0) {}

    bool Next() override {
        if (!m_child->next_tuple()) return false;
        m_buf.clear();
        m_len = m_sch->WritePayloadToBuffer(m_child->get_record(), m_buf);
        return true;
    }

    const char* GetCurrentItem(FieldOffset& p_reclen) override {
        p_reclen = m_len;
        return m_buf.data();
    }

    bool SupportsRewind() const override { return false; }
    uint64_t SavePosition() const override { return 0; }
    void Rewind(uint64_t) override {
        LOG(kFatal, "ChildItemIterator does not support rewind");
    }
    void EndScan() override {}

private:
    PlanExecState* m_child;
    const Schema* m_sch;
    maxaligned_char_buf m_buf;
    FieldOffset m_len;
};

}   // anonymous namespace

SortState::SortState(const Sort* plan,
                     std::unique_ptr<PlanExecState>&& child)
: PlanExecState(TAG(SortState), std::move(child))
, m_plan(plan)
, m_sorted(false)
{
}

SortState::~SortState() {
    if (m_initialized) close();
}

void
SortState::init() {
    get_child(0)->init();
    m_cur.clear();
    m_sorted = false;
    m_initialized = true;
}

void
SortState::DoSort() {
    ChildItemIterator input(get_child(0), m_plan->get_output_schema());
    m_sorter = ExternalSort::Create(m_plan->m_cmp, absl::GetFlag(FLAGS_sort_nways));
    m_iter = m_sorter->Sort(&input);
    m_sorted = true;
}

void
SortState::LoadCurrent() {
    FieldOffset len;
    const char* item = m_iter->GetCurrentItem(len);
    // Datums may point into the iterator's page, valid until its next Next()
    m_cur = m_plan->get_output_schema()->DissemblePayload(item);
}

bool
SortState::next_tuple() {
    if (!m_sorted) DoSort();
    m_cur.clear();
    if (!m_iter->Next()) return false;
    LoadCurrent();
    return true;
}

void
SortState::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
}

std::vector<NullableDatumRef>
SortState::get_record() {
    std::vector<NullableDatumRef> out;
    out.reserve(m_cur.size());
    for (const Datum& d : m_cur) out.emplace_back(d);
    return out;
}

void
SortState::close() {
    m_cur.clear();
    if (m_iter) {
        m_iter->EndScan();
        m_iter.reset();          // before m_sorter, which owns the tmp file
    }
    m_sorter.reset();
    m_sorted = false;
    get_child(0)->close();
    m_initialized = false;
}

void
SortState::rewind() {
    m_cur.clear();
    if (m_sorted) m_iter->Rewind(0);
}

Datum
SortState::save_position() const {
    return Datum::From((uint64_t) (m_iter ? m_iter->SavePosition() : 0));
}

bool
SortState::rewind(DatumRef saved_position) {
    if (!m_sorted) DoSort();
    uint64_t p = saved_position.GetUInt64();
    m_iter->Rewind(p);
    m_cur.clear();
    if (p == 0) return false;
    LoadCurrent();
    return true;
}

}    // namespace taco
