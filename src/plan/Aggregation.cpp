#include "plan/Aggregation.h"
#include "execution/AggregationState.h"
#include "catalog/CatCache.h"
#include "utils/builtin_funcs.h"

namespace taco {

Aggregation::Aggregation(std::unique_ptr<PlanNode>&& child,
                         std::vector<std::unique_ptr<ExprNode>>&& exprs,
                         const std::vector<AggType>& aggtyps)
: PlanNode(TAG(Aggregation), std::move(child))
, m_exprs(std::move(exprs))
, m_aggtyps(aggtyps)
{
    if (m_exprs.size() != m_aggtyps.size()) {
        LOG(kError, "Aggregation: %d exprs but %d aggtyps",
                    (int) m_exprs.size(), (int) m_aggtyps.size());
    }
    std::vector<Oid> typid;
    std::vector<uint64_t> typparam;
    std::vector<bool> nullable;
    for (size_t i = 0; i < m_exprs.size(); ++i) {
        Oid opr = m_exprs[i]->ReturnType()->typid();
        Oid aggid = g_catcache->FindAggregationByTidAndOprType(m_aggtyps[i], opr);
        if (aggid == InvalidOid) {
            LOG(kError, "no aggregation %d for type %u", (int) m_aggtyps[i], opr);
        }
        auto agg = g_catcache->FindAggregation(aggid);
        m_init.push_back(FindBuiltinFunction(agg->agginitfuncid()));
        m_acc.push_back(FindBuiltinFunction(agg->aggaccfuncid()));
        m_fin.push_back(FindBuiltinFunction(agg->aggfinalizefuncid()));
        typid.push_back(agg->aggrettypid());
        typparam.push_back(0);
        nullable.push_back(true);
    }
    m_schema.reset(Schema::Create(typid, typparam, nullable));
    m_schema->ComputeLayout();
}

std::unique_ptr<Aggregation>
Aggregation::Create(std::unique_ptr<PlanNode>&& child,
                    std::vector<std::unique_ptr<ExprNode>>&& exprs,
                    const std::vector<AggType>& aggtyps) {
    return absl::WrapUnique(new Aggregation(std::move(child), std::move(exprs), aggtyps));
}

Aggregation::~Aggregation() {}

void
Aggregation::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
}

std::unique_ptr<PlanExecState>
Aggregation::create_exec_state() const {
    std::unique_ptr<PlanExecState> child = get_child(0)->create_exec_state();
    return absl::WrapUnique(new AggregationState(this, std::move(child)));
}

const Schema*
Aggregation::get_output_schema() const {
    return m_schema.get();
}

}    // namespace taco
