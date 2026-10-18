#ifndef EXECUTION_TEMPTABLESTATE_H
#define EXECUTION_TEMPTABLESTATE_H

#include "execution/PlanExecState.h"
#include "plan/TempTable.h"

namespace taco {

class TempTableState: public PlanExecState {
public:
    ~TempTableState() override;

    void node_properties_to_string(std::string& buf, int indent) const override;

    void init() override;

    bool next_tuple() override;

    std::vector<NullableDatumRef> get_record() override;

    void close() override;

    void rewind() override;

    const PlanNode*
    get_plan() const override {
        return m_plan;
    }

    Datum save_position() const override;

    bool rewind(DatumRef saved_position) override;

private:
    TempTableState(const TempTable* plan);

    const TempTable* m_plan;

    // 0 = before the first record; k = currently at m_records[k - 1]
    uint64_t m_pos;

    friend class TempTable;
};

}    // namespace taco

#endif
