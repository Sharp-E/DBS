#include "expr/BinaryOperator.h"
#include "utils/builtin_funcs.h"
#include "catalog/CatCache.h"

namespace taco {

BinaryOperator::BinaryOperator(OpType optype,
                               std::unique_ptr<ExprNode>&& left,
                               std::unique_ptr<ExprNode>&& right)
: ExprNode(TAG(BinaryOperator), std::move(left), std::move(right))
, m_optype(optype)
{
    if (!OpTypeIsBinary(optype)) {
        LOG(kError, "OpType %d does not name a binary operator.", optype);
    }
    auto func_oid = g_catcache->FindOperator(
        optype,
        get_input_as<ExprNode>(0)->ReturnType()->typid(),
        get_input_as<ExprNode>(1)->ReturnType()->typid());
    if (func_oid == InvalidOid) {
        LOG(kError, "no binary operator %s found for the operand types",
                    GetOpTypeSymbol(optype));
    }
    m_func = FindBuiltinFunction(func_oid);
    m_type =
        g_catcache->FindType(g_catcache->FindFunction(func_oid)->funcrettypid());
}

std::unique_ptr<BinaryOperator>
BinaryOperator::Create(OpType optype,
                       std::unique_ptr<ExprNode>&& left,
                       std::unique_ptr<ExprNode>&& right) {
    return absl::WrapUnique(
        new BinaryOperator(optype, std::move(left), std::move(right)));
}

// Shared by both Eval overloads. The operands are local Datums that die when
// this returns, so make sure the result never references them.
template<class Rec>
static Datum
EvalBinary(const FunctionInfo& func, const ExprNode* l, const ExprNode* r,
           Rec record) {
    Datum lhs = l->Eval(record);
    Datum rhs = r->Eval(record);
    if (lhs.isnull() || rhs.isnull()) {
        return Datum::FromNull();
    }
    Datum res = FunctionCall(func, lhs, rhs);
    if (res.HasExternalRef()) {
        return res.DeepCopy();
    }
    return res;
}

Datum
BinaryOperator::Eval(const std::vector<NullableDatumRef>& record) const {
    return EvalBinary<const std::vector<NullableDatumRef>&>(
        m_func, get_input_as<ExprNode>(0), get_input_as<ExprNode>(1), record);
}

Datum
BinaryOperator::Eval(const char* record) const {
    return EvalBinary<const char*>(
        m_func, get_input_as<ExprNode>(0), get_input_as<ExprNode>(1), record);
}

void
BinaryOperator::node_properties_to_string(std::string &buf, int indent) const {
    append_indent(buf, indent);
    absl::StrAppendFormat(&buf, "OP_TYPE = %s", GetOpTypeSymbol(m_optype));
}

}
