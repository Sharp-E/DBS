#include "expr/Variable.h"
#include "catalog/systables/initoids.h"
#include "catalog/CatCache.h"

namespace taco {

Variable::Variable(const Schema* schema, FieldId fid)
: ExprNode(TAG(Variable))
, m_schema(schema)
, m_fid(fid)
{
    if (!schema) {
        LOG(kError, "schema must not be null for a Variable");
    }
    if ((size_t) fid >= (size_t) schema->GetNumFields()) {
        LOG(kError, "field id %d out of range (schema has %d fields)",
                    (int) fid, (int) schema->GetNumFields());
    }
    auto typ = g_catcache->FindType(schema->GetFieldTypeId(fid));
    if (!typ) {
        LOG(kError, "field %d has an invalid type", (int) fid);
    }
    m_type = typ;
}

std::unique_ptr<Variable>
Variable::Create(const Schema* schema, FieldId fid) {
    return absl::WrapUnique(new Variable(schema, fid));
}

Datum
Variable::Eval(const std::vector<NullableDatumRef>& record) const {
    // the returned Datum must own its memory
    return record[m_fid].DeepCopy(m_schema->FieldPassByRef(m_fid));
}

Datum
Variable::Eval(const char* record) const {
    // GetField may reference the record buffer; make an owned copy
    return m_schema->GetField(m_fid, record).DeepCopy();
}

void
Variable::node_properties_to_string(std::string& buf, int indent) const {
    append_indent(buf, indent);
    absl::StrAppendFormat(&buf, "FIELD_ID = %d", (int) m_fid);
}

}
