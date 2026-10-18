#include "extsort/ExternalSort.h"
#include <queue>

namespace taco {

ExternalSort::ExternalSort(const SortCompareFunction& comp, size_t merge_ways)
{
    // TODO: implement it.
}

ExternalSort::~ExternalSort() {
    // TODO: implement it.
}

std::unique_ptr<ExternalSort>
ExternalSort::Create(const SortCompareFunction& comp, size_t merge_ways) {
    return absl::WrapUnique(new ExternalSort(comp, merge_ways));
}

void
ExternalSort::SortInitialRuns(ItemIterator* input_iter) {
    // TODO: implement it
}

PageNumber
ExternalSort::MergeInternalRuns(size_t low_run, size_t high_run) {
    // TODO: implement it.
    return 0;
}

void
ExternalSort::GenerateNewPass(std::vector<PageNumber>& new_run_boundaries) {
    // TODO: implement it.
}

std::unique_ptr<ItemIterator>
ExternalSort::Sort(ItemIterator* input_iterator) {
    // TODO: implement it.
    return nullptr;
}

ExternalSort::OutputIterator::OutputIterator(const File* tmpfile,
                                             PageNumber final_num_pages)
{
    // TODO: implement it.
}

ExternalSort::OutputIterator::~OutputIterator() {
    // TODO: implement it.
}

bool
ExternalSort::OutputIterator::Next() {
    // TODO: implement it.
    return false;
}

const char*
ExternalSort::OutputIterator::GetCurrentItem(FieldOffset& p_reclen) {
    // TODO: implement it.
    return nullptr;
}

void
ExternalSort::OutputIterator::Rewind(uint64_t pos) {
    // TODO: implement it.
}

void
ExternalSort::OutputIterator::EndScan() {
    // TODO: implement it
}

}
