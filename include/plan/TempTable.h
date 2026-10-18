#ifndef PLAN_TEMPTABLE_H
#define PLAN_TEMPTABLE_H

#include "plan/PlanNode.h"
#include "catalog/Schema.h"

namespace taco {

class TempTableState;

class TempTable: public PlanNode {
public:
    static std::unique_ptr<TempTable>
    Create(const Schema* schema);

    ~TempTable() override;

    void insert_record(std::vector<Datum>&& record);

    void node_properties_to_string(std::string& buf, int indent) const override;

    std::unique_ptr<PlanExecState> create_exec_state() const override;

    const Schema* get_output_schema() const override;

private:
    TempTable(const Schema* schema);

    const Schema* m_schema;
    std::vector<std::vector<Datum>> m_records;

    friend class TempTableState;
};

}    // namespace taco

#endif
