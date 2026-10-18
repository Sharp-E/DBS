#include "extsort/ExternalSort.h"
#include <queue>
#include <algorithm>
#include <cstring>
#include "dbmain/Database.h"

namespace taco {

ExternalSort::ExternalSort(const SortCompareFunction& comp, size_t merge_ways)
: m_current_pass(0), m_merge_ways(merge_ways), m_cmp(comp), m_write_pid(0)
{
    if (merge_ways < 2) {
        LOG(kError, "ExternalSort requires merge_ways >= 2, got %d", (int) merge_ways);
    }
    m_buf = unique_aligned_alloc(512, (merge_ways + 1) * PAGE_SIZE);
    memset(m_buf.get(), 0, (merge_ways + 1) * PAGE_SIZE);
    m_file[0] = g_fileman->Open(NEW_TMP_FID);
    m_file[1] = g_fileman->Open(NEW_TMP_FID);
    m_npages[0] = 0;
    m_npages[1] = 0;
}

ExternalSort::~ExternalSort() {
}

std::unique_ptr<ExternalSort>
ExternalSort::Create(const SortCompareFunction& comp, size_t merge_ways) {
    return absl::WrapUnique(new ExternalSort(comp, merge_ways));
}

void
ExternalSort::AppendPage(uint8_t fidx, const char* buf) {
    PageNumber pid = m_write_pid++;
    while (pid >= m_npages[fidx]) {
        m_file[fidx]->AllocatePage();
        ++m_npages[fidx];
    }
    m_file[fidx]->WritePage(pid, buf);
}

void
ExternalSort::SortInitialRuns(ItemIterator* input_iter) {
    char* area = (char*) m_buf.get();
    const size_t cap = m_merge_ways * PAGE_SIZE;
    char* outbuf = GetBuf(m_merge_ways);
    std::vector<std::pair<const char*, FieldOffset>> items;
    size_t used = 0;

    m_write_pid = 0;
    m_run_boundaries.clear();
    m_run_boundaries.push_back(0);

    auto flush_run = [&]() {
        if (items.empty()) return;
        std::sort(items.begin(), items.end(),
            [this](const std::pair<const char*, FieldOffset>& a,
                   const std::pair<const char*, FieldOffset>& b) {
                return m_cmp(a.first, b.first) < 0;
            });
        VarlenDataPage::InitializePage(outbuf, 0);
        VarlenDataPage dp(outbuf);
        bool has = false;
        for (auto& it : items) {
            Record rec(it.first, it.second);
            if (!dp.InsertRecord(rec)) {
                LOG(kError, "item of length %d is too large", (int) it.second);
            }
            if (rec.GetRecordId().sid == INVALID_SID) {
                AppendPage(0, outbuf);
                VarlenDataPage::InitializePage(outbuf, 0);
                dp.InsertRecord(rec);
            }
            has = true;
        }
        if (has) AppendPage(0, outbuf);
        m_run_boundaries.push_back(m_write_pid);
        items.clear();
        used = 0;
    };

    while (input_iter->Next()) {
        FieldOffset len;
        const char* data = input_iter->GetCurrentItem(len);
        size_t alen = MAXALIGN((size_t) len);
        if (alen > cap) {
            LOG(kError, "item of length %d does not fit in sort memory", (int) len);
        }
        if (used + alen > cap) {
            flush_run();
        }
        memcpy(area + used, data, len);
        items.emplace_back(area + used, len);
        used += alen;
    }
    flush_run();
}

PageNumber
ExternalSort::MergeInternalRuns(size_t low_run, size_t high_run) {
    struct RunState { PageNumber pid, end; SlotId sid, maxsid; char* buf; };
    const size_t k = high_run - low_run + 1;
    const uint8_t in = 1 - m_current_pass;
    std::vector<RunState> rs(k);

    auto cur = [&](size_t j, FieldOffset* len) -> const char* {
        return VarlenDataPage(rs[j].buf).GetRecordBuffer(rs[j].sid, len);
    };
    auto cmp = [&](size_t a, size_t b) {
        int c = m_cmp(cur(a, nullptr), cur(b, nullptr));
        if (c != 0) return c > 0;   // smaller item = higher priority
        return a > b;
    };
    std::priority_queue<size_t, std::vector<size_t>, decltype(cmp)> pq(cmp);

    for (size_t j = 0; j < k; ++j) {
        RunState& r = rs[j];
        r.pid = m_run_boundaries[low_run + j];
        r.end = m_run_boundaries[low_run + j + 1];
        r.buf = GetBuf(j);
        r.sid = 0;
        r.maxsid = 0;
        if (r.pid < r.end) {
            m_file[in]->ReadPage(r.pid, r.buf);
            r.maxsid = VarlenDataPage(r.buf).GetMaxSlotId();
            r.sid = MinSlotId;
            if (r.maxsid >= MinSlotId) pq.push(j);
        }
    }

    char* outbuf = GetBuf(m_merge_ways);
    VarlenDataPage::InitializePage(outbuf, 0);
    VarlenDataPage out(outbuf);
    bool has = false;

    while (!pq.empty()) {
        size_t j = pq.top();
        pq.pop();
        FieldOffset len;
        const char* data = cur(j, &len);
        Record rec(data, len);
        out.InsertRecord(rec);
        if (rec.GetRecordId().sid == INVALID_SID) {
            AppendPage(m_current_pass, outbuf);
            VarlenDataPage::InitializePage(outbuf, 0);
            out.InsertRecord(rec);
        }
        has = true;

        // advance run j (only after the item was copied out)
        RunState& r = rs[j];
        if (r.sid < r.maxsid) {
            ++r.sid;
            pq.push(j);
        } else if (r.pid + 1 < r.end) {
            ++r.pid;
            m_file[in]->ReadPage(r.pid, r.buf);
            r.maxsid = VarlenDataPage(r.buf).GetMaxSlotId();
            r.sid = MinSlotId;
            if (r.maxsid >= MinSlotId) pq.push(j);
        }
    }
    if (has) AppendPage(m_current_pass, outbuf);
    return m_write_pid;
}

void
ExternalSort::GenerateNewPass(std::vector<PageNumber>& new_run_boundaries) {
    new_run_boundaries.clear();
    new_run_boundaries.push_back(0);
    m_write_pid = 0;
    size_t nruns = m_run_boundaries.size() - 1;
    for (size_t low = 0; low < nruns; low += m_merge_ways) {
        size_t high = std::min(low + m_merge_ways, nruns) - 1;
        new_run_boundaries.push_back(MergeInternalRuns(low, high));
    }
}

std::unique_ptr<ItemIterator>
ExternalSort::Sort(ItemIterator* input_iterator) {
    SortInitialRuns(input_iterator);     // runs now in m_file[0]
    input_iterator->EndScan();
    m_current_pass = 1;                  // next pass writes to m_file[1]
    while (m_run_boundaries.size() - 1 > 1) {
        std::vector<PageNumber> nb;
        GenerateNewPass(nb);
        m_run_boundaries = std::move(nb);
        m_current_pass = 1 - m_current_pass;
    }
    uint8_t final_f = 1 - m_current_pass;
    PageNumber npages = m_run_boundaries.back();
    return absl::WrapUnique(new OutputIterator(m_file[final_f].get(), npages));
}

ExternalSort::OutputIterator::OutputIterator(const File* tmpfile,
                                             PageNumber final_num_pages)
: m_file(tmpfile), m_npages(final_num_pages),
  m_buf(unique_aligned_alloc(512, PAGE_SIZE)),
  m_pid(0), m_sid(0), m_loaded(false)
{
}

ExternalSort::OutputIterator::~OutputIterator() {
    EndScan();
}

void
ExternalSort::OutputIterator::LoadPage(PageNumber pid) {
    m_file->ReadPage(pid, (char*) m_buf.get());
    m_pid = pid;
    m_loaded = true;
}

bool
ExternalSort::OutputIterator::Next() {
    if (!m_buf || m_npages == 0) return false;
    if (!m_loaded) {
        LoadPage(0);
        m_sid = 0;
    }
    for (;;) {
        VarlenDataPage dp((char*) m_buf.get());
        if (m_sid < dp.GetMaxSlotId()) {
            ++m_sid;
            return true;
        }
        if (m_pid + 1 >= m_npages) return false;
        LoadPage(m_pid + 1);
        m_sid = 0;
    }
}

const char*
ExternalSort::OutputIterator::GetCurrentItem(FieldOffset& p_reclen) {
    return VarlenDataPage((char*) m_buf.get()).GetRecordBuffer(m_sid, &p_reclen);
}

void
ExternalSort::OutputIterator::Rewind(uint64_t pos) {
    if (!m_buf) m_buf = unique_aligned_alloc(512, PAGE_SIZE);
    if (pos == 0) {
        m_loaded = false;
        m_pid = 0;
        m_sid = 0;
        return;
    }
    PageNumber pid = (PageNumber) (pos >> 32);
    SlotId sid = (SlotId) (pos & 0xFFFFFFFFu);
    if (!m_loaded || m_pid != pid) LoadPage(pid);
    m_sid = sid;
}

void
ExternalSort::OutputIterator::EndScan() {
    m_buf.reset();
    m_loaded = false;
    m_pid = 0;
    m_sid = 0;
}

}
