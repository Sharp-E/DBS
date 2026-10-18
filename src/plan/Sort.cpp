#include "plan/Sort.h"
#include "catalog/CatCache.h"
#include "utils/builtin_funcs.h"
#include "expr/optypes.h"
#include "execution/SortState.h"

using namespace std::placeholders;

namespace taco {

Sort::Sort(std::unique_ptr<PlanNode>&& child,
           std::vector<std::unique_ptr<ExprNode>>&& sort_exprs,
           const std::vector<bool>& desc)
: PlanNode(TAG(Sort), std::move(child))
, m_cmp(std::bind(&Sort::sort_compare_func_impl, this, _1, _2))
, m_exprs(std::move(sort_exprs))
, m_desc(desc)
{
    if (m_exprs.size() != m_desc.size()) {
        LOG(kError, "Sort: %d sort exprs but %d desc flags",
                    (int) m_exprs.size(), (int) m_desc.size());
    }
    for (const auto& e : m_exprs) {
        Oid t = e->ReturnType()->typid();
        Oid fid = g_catcache->FindOperator(OPTYPE_LT, t, t);
        if (fid == InvalidOid) {
            LOG(kError, "no < operator for type %u", t);
        }
        m_lt.push_back(FindBuiltinFunction(fid));
    }
}

int
Sort::sort_compare_func_impl(const char *a, const char *b) const {
    for (size_t i = 0; i < m_exprs.size(); ++i) {
        Datum da = m_exprs[i]->Eval(a);
        Datum db = m_exprs[i]->Eval(b);
        int c;
        if (da.isnull() || db.isnull()) {
            // NULL sorts before any non-NULL value
            c = (da.isnull() ? 0 : 1) - (db.isnull() ? 0 : 1);
        } else if (FunctionCall(m_lt[i], da, db).GetBool()) {
            c = -1;
        } else if (FunctionCall(m_lt[i], db, da).GetBool()) {
            c = 1;
        } else {
            c = 0;
        }
        if (m_desc[i]) c = -c;
        if (c != 0) return c;
    }
    return 0;
}

std::unique_ptr<Sort>
Sort::Create(std::unique_ptr<PlanNode>&& child,
             std::vector<std::unique_ptr<ExprNode>>&& sort_exprs,
             const std::vector<bool>& desc) {
    return absl::WrapUnique(
        new Sort(std::move(child), std::move(sort_exprs), desc));
}

Sort::~Sort() {
}

void
Sort::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
}

std::unique_ptr<PlanExecState>
Sort::create_exec_state() const {
    std::unique_ptr<PlanExecState> child = get_child(0)->create_exec_state();
    return absl::WrapUnique(new SortState(this, std::move(child)));
}

const Schema*
Sort::get_output_schema() const {
    return get_child(0)->get_output_schema();
}

}
