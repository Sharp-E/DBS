#include "expr/BinaryOperator.h"
#include "utils/builtin_funcs.h"
#include "catalog/CatCache.h"

namespace taco {

BinaryOperator::BinaryOperator(OpType optype,
                               std::unique_ptr<ExprNode>&& left,
                               std::unique_ptr<ExprNode>&& right)
: ExprNode(TAG(BinaryOperator), std::move(left), std::move(right))
{
    // TODO implement me
}

std::unique_ptr<BinaryOperator>
BinaryOperator::Create(OpType optype,
                       std::unique_ptr<ExprNode>&& left,
                       std::unique_ptr<ExprNode>&& right) {
    return absl::WrapUnique(
        new BinaryOperator(optype, std::move(left), std::move(right)));
}

Datum
BinaryOperator::Eval(const std::vector<NullableDatumRef>& record) const {
    // TODO implement me
    return Datum::FromNull();
}

Datum
BinaryOperator::Eval(const char* record) const {
    // TODO implement me
    return Datum::FromNull();
}

void
BinaryOperator::node_properties_to_string(std::string &buf, int indent) const {
    append_indent(buf, indent);
}

}
