#include "expr/Variable.h"
#include "catalog/systables/initoids.h"
#include "catalog/CatCache.h"

namespace taco {

Variable::Variable(const Schema* schema, FieldId fid)
: ExprNode(TAG(Variable))
{
    // TODO implement me
}

std::unique_ptr<Variable>
Variable::Create(const Schema* schema, FieldId fid) {
    return absl::WrapUnique(new Variable(schema, fid));
}

Datum
Variable::Eval(const std::vector<NullableDatumRef>& record) const {
    // TODO implement me
    return Datum::FromNull();
}

Datum
Variable::Eval(const char* record) const {
    // TODO implement me
    return Datum::FromNull();
}

void
Variable::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
}

}
