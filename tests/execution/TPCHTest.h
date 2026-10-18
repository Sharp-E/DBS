#ifndef TESTS_EXECUTION_TPCHTEST_H
#define TESTS_EXECUTION_TPCHTEST_H
#include "base/TDBDBTest.h"

#include <absl/flags/declare.h>
#include <absl/container/flat_hash_map.h>

#include "catalog/CatCache.h"
#include "catalog/IndexDesc.h"
#include "catalog/TableDesc.h"
#include "catalog/Schema.h"
#include "catalog/systables.h"
#include "catalog/systables/initoids.h"
#include "storage/Record.h"
#include "plan/TableScan.h"
#include "execution/PlanExecState.h"
#include "expr/ExprNode.h"
#include "expr/optypes.h"
#include "expr/Variable.h"

ABSL_DECLARE_FLAG(std::string, test_db_path);

namespace taco {

extern bool g_is_data_loader;

class TPCHTest: public TDBDBTest {
protected:
    using P = std::unique_ptr<PlanNode>;
    using E = std::unique_ptr<ExprNode>;
    using ES = std::unique_ptr<PlanExecState>;

    void SetUp() override;

    void TearDown() override;

    void CreateAndLoadTPCHTables();

    void PrepareTablesAndIndexes();

    void LoadTable(absl::string_view tabname,
                   absl::string_view rawdata_filepath);

    template<class SomeDatum>
    std::string
    RecordToString(const std::vector<SomeDatum> &data,
                   const Schema *schema,
                   const char delim = ',') {
        std::string res;
        res.reserve(1024);
        ASSERT(data.size() == (size_t) schema->GetNumFields());
        for (FieldId i = 0; i < schema->GetNumFields(); ++i) {
            if (i != 0) {
                res.push_back(delim);
            }
            if (!data[i].isnull()) {
                const FunctionInfo &f =
                    m_typid2outfunc.at(schema->GetFieldTypeId(i));
                Datum outstr = FunctionCall(f, data[i]);
                ASSERT(!outstr.isnull());
                absl::string_view s = outstr.GetVarlenAsStringView();
                res.append(s.data(), s.length());
            }
        }
        return res;
    }

    inline std::string
    RecordToString(const Record &rec,
                   const Schema *schema) {
        std::vector<Datum> data = schema->DissemblePayload(rec.GetData());
        return RecordToString(data, schema);
    }

    /*!
     * Returns a simple boolean selection predicate `lhs_i op rhs`.
     *
     * The right hand side (rhs) of the predicate is created by calling the
     * input function of its type, as determined below:
     *   - If \p rhs_typid is InvadlidOid, the type of the rhs is the same as
     *   the field on the left hand side.
     *   - Otherwise, the type is specified by \p rhs_typid.
     *
     * The boolean operator used in the predicate is the default one for the
     * given \p optype and types of both sides of the predicate. It is an error
     * if there is no such an operator and this function does not attempt any
     * implicit conversion on either side.
     *
     * It is also an error if \p lhs_i is out of range.
     */
    E SelPred(const Schema *schema,
              FieldId lhs_i,
              OpType optype,
              absl::string_view rhs_str,
              Oid rhs_typid = InvalidOid);

    /*!
     * Same as the other overload of `SelPred` except that the left hand side
     * is specified by an unambiguous field name. Note that all field names are
     * prefixed with a table abbreviation in TPC-H, so it is usually Ok to find
     * the field by name. However, aggregation columns do not have names and
     * self-joins will create a schema with duplicate field names, where it is
     * not a good idea to get the field id by name: use field ID instead.
     */
    E
    SelPred(const Schema *schema,
            absl::string_view fieldname_i,
            OpType optype,
            absl::string_view rhs_str,
            Oid rhs_typid = InvalidOid) {
        FieldId i = schema->GetFieldIdFromFieldName(fieldname_i);
        if (i == InvalidFieldId) {
            LOG(kError, "field %s does not exist in the provided schema",
                        fieldname_i);
        }
        return SelPred(schema, i, optype, rhs_str, rhs_typid);
    }

    /*!
     * Returns a join predicate `lhs_i op rhs_j`.
     *
     * It is an error if \p lhs_i or \p rhs_j is out of bound or the operator
     * does not exist.
     */
    E JoinPred(const Schema *schema,
               FieldId lhs_i,
               OpType optype,
               FieldId rhs_j);

    /*!
     * Same as the other overload of `JoinPred` except that the left hand side
     * and the right hand side are specified by an unambiguous field name. Note
     * that all field names are prefixed with a table abbreviation in TPC-H, so
     * it is usually Ok to find the field by name. However, aggregation columns
     * do not have names and self-joins will create a schema with duplicate
     * field names, where it is not a good idea to get the field id by name:
     * use field ID instead.
     */
    E
    JoinPred(const Schema *schema,
             absl::string_view fieldname_i,
             OpType optype,
             absl::string_view fieldname_j) {
        FieldId i = schema->GetFieldIdFromFieldName(fieldname_i);
        if (i == InvalidFieldId) {
            LOG(kError, "field %s does not exist in the provided schema",
                        fieldname_i);
        }
        FieldId j = schema->GetFieldIdFromFieldName(fieldname_j);
        if (j == InvalidFieldId) {
            LOG(kError, "field %s does not exist in the provided schema",
                        fieldname_i);
        }
        return JoinPred(schema, i, optype, j);
    }

    /*!
     * Returns a variable that references the given field in the given schema.
     *
     * Note that the field name must be unique within the schema for this
     * function to work. If not, use the positional notation (field id) to
     * directly create the Variable expression.
     */
    E
    FieldRef(const Schema *schema,
             absl::string_view fieldname) {
        FieldId i = schema->GetFieldIdFromFieldName(fieldname);
        if (i == InvalidFieldId) {
            LOG(kError, "field %s does not exist in the provided schema",
                        fieldname);
        }
        return Variable::Create(schema, i);
    }

    virtual std::string
    AnsSubDir() const {
        return "";
    }

    void RunPlan(const PlanNode *p);

    std::shared_ptr<const TableDesc> m_region;
    std::shared_ptr<const TableDesc> m_nation;
    std::shared_ptr<const TableDesc> m_part;
    std::shared_ptr<const TableDesc> m_supplier;
    std::shared_ptr<const TableDesc> m_partsupp;
    std::shared_ptr<const TableDesc> m_customer;
    std::shared_ptr<const TableDesc> m_orders;
    std::shared_ptr<const TableDesc> m_lineitem;

    // primary indexes
    std::shared_ptr<const IndexDesc> m_idx_r_regionkey;
    std::shared_ptr<const IndexDesc> m_idx_n_nationkey;
    std::shared_ptr<const IndexDesc> m_idx_p_partkey;
    std::shared_ptr<const IndexDesc> m_idx_s_suppkey;
    std::shared_ptr<const IndexDesc> m_idx_ps_partkey_ps_suppkey;
    std::shared_ptr<const IndexDesc> m_idx_c_custkey;
    std::shared_ptr<const IndexDesc> m_idx_o_orderkey;
    std::shared_ptr<const IndexDesc> m_idx_l_orderkey_l_linenumber;

    // secondary indexes
    std::shared_ptr<const IndexDesc> m_idx_o_custkey;
    std::shared_ptr<const IndexDesc> m_idx_o_orderdate;
    std::shared_ptr<const IndexDesc> m_idx_l_suppkey;
    std::shared_ptr<const IndexDesc> m_idx_l_receiptdate;
    std::shared_ptr<const IndexDesc> m_idx_l_shipdate;
    std::shared_ptr<const IndexDesc> m_idx_l_linestatus_l_shipdate;
    std::shared_ptr<const IndexDesc> m_idx_l_orderkey_l_receiptdate;

    absl::flat_hash_map<Oid, FunctionInfo> m_typid2infunc;
    absl::flat_hash_map<Oid, FunctionInfo> m_typid2outfunc;

    static constexpr const Oid INT4 = initoids::TYP_INT4;
    static constexpr const Oid INT8 = initoids::TYP_INT8;
    static constexpr const Oid VARCHAR = initoids::TYP_VARCHAR;
    static constexpr const Oid CHAR = initoids::TYP_CHAR;
    static constexpr const Oid DOUBLE = initoids::TYP_DOUBLE;
    static constexpr const Oid DATE = initoids::TYP_DATE;
};

}

#endif      // TESTS_EXECUTION_TPCHTEST_H
