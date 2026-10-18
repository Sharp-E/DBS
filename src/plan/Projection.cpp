#include "plan/Projection.h"
#include "execution/ProjectionState.h"
#include "expr/Variable.h"

namespace taco {

Projection::Projection(std::unique_ptr<PlanNode>&& child,
                       std::vector<std::unique_ptr<ExprNode>>&& exprs)
: PlanNode(TAG(Projection), std::move(child))
, m_exprs(std::move(exprs))
{
    const Schema* child_schema = get_child(0)->get_output_schema();

    std::vector<Oid> typid;
    std::vector<uint64_t> typparam;
    std::vector<bool> nullable;
    typid.reserve(m_exprs.size());
    typparam.reserve(m_exprs.size());
    nullable.reserve(m_exprs.size());

    for (const auto& expr : m_exprs) {
        if (!expr) {
            LOG(kError, "Projection got a null expression");
        }
        typid.push_back(expr->ReturnType()->typid());
        if (expr->get_tag() == TAG(Variable)) {
            // a plain column reference keeps the column's type param/nullness
            FieldId fid = static_cast<const Variable*>(expr.get())->GetFieldId();
            typparam.push_back(child_schema->GetFieldTypeParam(fid));
            nullable.push_back(child_schema->FieldIsNullable(fid));
        } else {
            typparam.push_back(0);
            nullable.push_back(true);
        }
    }

    m_schema.reset(Schema::Create(typid, typparam, nullable));
    m_schema->ComputeLayout();
}

std::unique_ptr<Projection>
Projection::Create(std::unique_ptr<PlanNode>&& child,
                   std::vector<std::unique_ptr<ExprNode>>&& exprs) {

    return absl::WrapUnique(new Projection(std::move(child), std::move(exprs)));
}

Projection::~Projection() {
}

void
Projection::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
    absl::StrAppendFormat(&buf, "NUM_EXPRS = %d", (int) m_exprs.size());
}

std::unique_ptr<PlanExecState> Projection::create_exec_state() const {
    std::unique_ptr<PlanExecState> child = get_child(0)->create_exec_state();
    return absl::WrapUnique(new ProjectionState(this, std::move(child)));
}

const Schema*
Projection::get_output_schema() const {
    return m_schema.get();
}

}
