#include "base/TDBDBTest.h"

#include "catalog/CatCache.h"
#include "utils/builtin_funcs.h"
#include "expr/Literal.h"
#include "expr/Variable.h"
#include "expr/BinaryOperator.h"
#include "expr/UnaryOperator.h"
#include "expr/AndOperator.h"
#include "expr/OrOperator.h"
#include "expr/Cast.h"
#include "expr/FuncCallOperator.h"

namespace taco {

const char* str_literal1 = "hello world!!";
const char* str_literal2 = "Welcome to TacoDB....";

class BasicTestExpression: public TDBDBTest {
protected:
    void
    SetUp() override {
        TDBDBTest::SetUp();

        m_schema = absl::WrapUnique(
            Schema::Create({initoids::TYP_INT4, initoids::TYP_UINT8,
                            initoids::TYP_DOUBLE, initoids::TYP_VARCHAR,
                            initoids::TYP_CHAR},
                           {0, 0, 0, 40, 21},
                           {false, false, false, false, true}));

        m_schema->ComputeLayout();


        m_data.emplace_back(Datum::From((int32_t)132));
        m_data.emplace_back(Datum::From((int64_t)384));
        m_data.emplace_back(Datum::From((double)3.1415926));


        auto varchar_typ = g_catcache->FindType(initoids::TYP_VARCHAR);
        ASSERT_NE(varchar_typ, nullptr);
        Oid varchar_in_oid = varchar_typ->typinfunc();
        m_varchar_in = FindBuiltinFunction(varchar_in_oid);
        m_data.emplace_back(FunctionCallWithTypparam(m_varchar_in,
            m_schema->GetFieldTypeParam(3),
            Datum::FromCString(str_literal1)));


        auto char_typ = g_catcache->FindType(initoids::TYP_CHAR);
        ASSERT_NE(char_typ, nullptr);
        Oid char_in_oid = char_typ->typinfunc();
        m_char_in = FindBuiltinFunction(char_in_oid);
        m_data.emplace_back(FunctionCallWithTypparam(m_char_in,
            m_schema->GetFieldTypeParam(4),
            Datum::FromCString(str_literal2)));

        for (auto& x: m_data) m_record.emplace_back(x);
        m_schema->WritePayloadToBuffer(m_record, m_rec_buf);

        Oid char_eq_oid = g_catcache->FindOperator(
            OPTYPE_EQ, char_typ->typid(), char_typ->typid());
        ASSERT_NE(char_eq_oid, InvalidOid);
        m_char_eq = FindBuiltinFunction(char_eq_oid);
        ASSERT_TRUE(m_char_eq);
    }

    void
    TearDown() override {
        TDBDBTest::TearDown();
    }

    std::unique_ptr<Schema> m_schema;
    std::vector<Datum> m_data;
    std::vector<NullableDatumRef> m_record;
    maxaligned_char_buf m_rec_buf;
    FunctionInfo m_char_in;
    FunctionInfo m_varchar_in;
    FunctionInfo m_char_eq;
};

TEST_F(BasicTestExpression, TestExpressionLiteral) {
    TDB_TEST_BEGIN

    std::unique_ptr<ExprNode> literal = Literal::Create(Datum::From(100), initoids::TYP_INT8);
    ASSERT_EQ(literal->ReturnType()->typid(), initoids::TYP_INT8);
    ASSERT_EQ(literal->Eval(nullptr).GetInt8(), 100);
    ASSERT_EQ(literal->Eval({}).GetInt64(), 100);

    TDB_TEST_END
}

TEST_F(BasicTestExpression, TestExpressionVariable) {
    TDB_TEST_BEGIN

    std::unique_ptr<ExprNode> var1 = Variable::Create(m_schema.get(), 0);
    std::unique_ptr<ExprNode> var2 = Variable::Create(m_schema.get(), 2);
    std::unique_ptr<ExprNode> var3 = Variable::Create(m_schema.get(), 3);
    std::unique_ptr<ExprNode> var4 = Variable::Create(m_schema.get(), 4);

    ASSERT_EQ(var1->ReturnType()->typid(), initoids::TYP_INT4);
    ASSERT_EQ(var1->Eval(m_record).GetInt32(), 132);
    ASSERT_EQ(var1->Eval(m_rec_buf.data()).GetInt32(), 132);

    ASSERT_EQ(var2->ReturnType()->typid(), initoids::TYP_DOUBLE);
    ASSERT_DOUBLE_EQ(var2->Eval(m_record).GetDouble(), 3.1415926);
    ASSERT_DOUBLE_EQ(var2->Eval(m_rec_buf.data()).GetDouble(), 3.1415926);

    ASSERT_EQ(var3->ReturnType()->typid(), initoids::TYP_VARCHAR);
    Datum v3_res1 = var3->Eval(m_record);
    ASSERT_EQ(v3_res1.GetVarlenAsStringView(), absl::string_view(str_literal1));
    ASSERT_FALSE(v3_res1.HasExternalRef());

    Datum v3_res2 = var3->Eval(m_rec_buf.data());
    ASSERT_EQ(v3_res2.GetVarlenAsStringView(), absl::string_view(str_literal1));
    ASSERT_FALSE(v3_res2.HasExternalRef());

    ASSERT_EQ(var4->ReturnType()->typid(), initoids::TYP_CHAR);
    Datum res4 = var4->Eval(m_record);
    Datum expected_res4 = FunctionCallWithTypparam(m_char_in,
        strlen(str_literal2),
        Datum::FromCString(str_literal2));
    Datum cmpres_res4 = FunctionCall(m_char_eq, res4, expected_res4);
    ASSERT_FALSE(cmpres_res4.isnull());
    ASSERT_TRUE(cmpres_res4.GetBool());

    TDB_TEST_END
}

TEST_F(BasicTestExpression, TestExpressionBinaryOp) {
    TDB_TEST_BEGIN

    {
        std::unique_ptr<ExprNode> var1 = Variable::Create(m_schema.get(), 0);
        std::unique_ptr<ExprNode> var2 = Variable::Create(m_schema.get(), 0);
        std::unique_ptr<ExprNode> add = BinaryOperator::Create(OPTYPE_ADD, std::move(var1), std::move(var2));

        ASSERT_EQ(add->ReturnType()->typid(), initoids::TYP_INT4);
        ASSERT_EQ(add->Eval(m_record).GetInt32(), 264);
        ASSERT_EQ(add->Eval(m_rec_buf.data()).GetInt32(), 264);
    }

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 0);
        std::unique_ptr<ExprNode> lit = Literal::Create(Datum::From((int32_t)3), initoids::TYP_INT4);
        std::unique_ptr<ExprNode> sub = BinaryOperator::Create(OPTYPE_SUB, std::move(var), std::move(lit));

        ASSERT_EQ(sub->ReturnType()->typid(), initoids::TYP_INT4);
        ASSERT_EQ(sub->Eval(m_record).GetInt32(), 129);
        ASSERT_EQ(sub->Eval(m_rec_buf.data()).GetInt32(), 129);
    }

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 2);
        std::unique_ptr<ExprNode> lit = Literal::Create(Datum::From((double)1.5), initoids::TYP_DOUBLE);
        std::unique_ptr<ExprNode> mul = BinaryOperator::Create(OPTYPE_MUL, std::move(var), std::move(lit));
        ASSERT_EQ(mul->ReturnType()->typid(), initoids::TYP_DOUBLE);
        ASSERT_DOUBLE_EQ(mul->Eval(m_record).GetDouble(), 4.7123889);
        ASSERT_DOUBLE_EQ(mul->Eval(m_rec_buf.data()).GetDouble(), 4.7123889);
    }

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 0);
        std::unique_ptr<ExprNode> lit = Literal::Create(Datum::From((int32_t)5), initoids::TYP_INT4);
        std::unique_ptr<ExprNode> div = BinaryOperator::Create(OPTYPE_DIV, std::move(var), std::move(lit));
        ASSERT_EQ(div->ReturnType()->typid(), initoids::TYP_INT4);
        ASSERT_EQ(div->Eval(m_record).GetInt32(), 26);
        ASSERT_EQ(div->Eval(m_rec_buf.data()).GetInt32(), 26);
    }

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 0);
        std::unique_ptr<ExprNode> lit = Literal::Create(Datum::From((int32_t)5), initoids::TYP_INT4);
        std::unique_ptr<ExprNode> mod = BinaryOperator::Create(OPTYPE_MOD, std::move(var), std::move(lit));
        ASSERT_EQ(mod->ReturnType()->typid(), initoids::TYP_INT4);
        ASSERT_EQ(mod->Eval(m_record).GetInt32(), 2);
        ASSERT_EQ(mod->Eval(m_rec_buf.data()).GetInt32(), 2);
    }

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 0);
        std::unique_ptr<ExprNode> lit = Literal::Create(Datum::From((int32_t)100), initoids::TYP_INT4);
        std::unique_ptr<ExprNode> lt = BinaryOperator::Create(OPTYPE_LT, std::move(var), std::move(lit));
        ASSERT_EQ(lt->ReturnType()->typid(), initoids::TYP_BOOL);
        ASSERT_EQ(lt->Eval(m_record).GetBool(), false);
        ASSERT_EQ(lt->Eval(m_rec_buf.data()).GetBool(), false);
    }

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 2);
        std::unique_ptr<ExprNode> lit = Literal::Create(Datum::From((double)1.414), initoids::TYP_DOUBLE);
        std::unique_ptr<ExprNode> ge = BinaryOperator::Create(OPTYPE_GE, std::move(var), std::move(lit));
        ASSERT_EQ(ge->ReturnType()->typid(), initoids::TYP_BOOL);
        ASSERT_EQ(ge->Eval(m_record).GetBool(), true);
        ASSERT_EQ(ge->Eval(m_rec_buf.data()).GetBool(), true);
    }

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 0);
        std::unique_ptr<ExprNode> lit = Literal::Create(Datum::From((int32_t)7), initoids::TYP_INT4);
        std::unique_ptr<ExprNode> expr = BinaryOperator::Create(OPTYPE_BITAND, std::move(var), std::move(lit));
        ASSERT_EQ(expr->ReturnType()->typid(), initoids::TYP_INT4);
        ASSERT_EQ(expr->Eval(m_record).GetInt32(), 4);
        ASSERT_EQ(expr->Eval(m_rec_buf.data()).GetInt32(), 4);
    }

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 1);
        std::unique_ptr<ExprNode> lit = Literal::Create(Datum::From((uint8_t)5), initoids::TYP_UINT1);
        std::unique_ptr<ExprNode> expr = BinaryOperator::Create(OPTYPE_RSH, std::move(var), std::move(lit));
        ASSERT_EQ(expr->ReturnType()->typid(), initoids::TYP_UINT8);
        ASSERT_EQ(expr->Eval(m_record).GetUInt64(), 12);
        ASSERT_EQ(expr->Eval(m_rec_buf.data()).GetUInt64(), 12);
    }

    TDB_TEST_END
}

TEST_F(BasicTestExpression, TestExpressionUnaryOp) {
    TDB_TEST_BEGIN

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 0);
        std::unique_ptr<ExprNode> expr = UnaryOperator::Create(OPTYPE_NEG, std::move(var));
        ASSERT_EQ(expr->Eval(m_record).GetInt32(), -132);
        ASSERT_EQ(expr->Eval(m_rec_buf.data()).GetInt32(), -132);
    }

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 1);
        std::unique_ptr<ExprNode> expr = UnaryOperator::Create(OPTYPE_BITNOT, std::move(var));
        ASSERT_EQ(expr->Eval(m_record).GetUInt64(), ~384);
        ASSERT_EQ(expr->Eval(m_rec_buf.data()).GetUInt64(), ~384);
    }

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 2);
        std::unique_ptr<ExprNode> lit = Literal::Create(Datum::From((double)1.414), initoids::TYP_DOUBLE);
        std::unique_ptr<ExprNode> ge = BinaryOperator::Create(OPTYPE_GE, std::move(var), std::move(lit));
        std::unique_ptr<ExprNode> expr = UnaryOperator::Create(OPTYPE_NOT, std::move(ge));
        ASSERT_EQ(expr->Eval(m_record).GetBool(), false);
        ASSERT_EQ(expr->Eval(m_rec_buf.data()).GetBool(), false);
    }

    TDB_TEST_END
}

TEST_F(BasicTestExpression, TestExpressionAndOr) {
    TDB_TEST_BEGIN

    {
        std::unique_ptr<ExprNode> lit1 = Literal::Create(Datum::From(false), initoids::TYP_BOOL);
        std::unique_ptr<ExprNode> lit2 = Literal::Create(Datum::FromNull(), initoids::TYP_BOOL);
        std::unique_ptr<ExprNode> expr = AndOperator::Create(std::move(lit1), std::move(lit2));
        ASSERT_EQ(expr->Eval(m_record).GetBool(), false);
        ASSERT_EQ(expr->Eval(m_rec_buf.data()).GetBool(), false);
    }

    {
        std::unique_ptr<ExprNode> lit1 = Literal::Create(Datum::From(true), initoids::TYP_BOOL);
        std::unique_ptr<ExprNode> lit2 = Literal::Create(Datum::FromNull(), initoids::TYP_BOOL);
        std::unique_ptr<ExprNode> expr = OrOperator::Create(std::move(lit1), std::move(lit2));
        ASSERT_EQ(expr->Eval(m_record).GetBool(), true);
        ASSERT_EQ(expr->Eval(m_rec_buf.data()).GetBool(), true);
    }

    {
        std::unique_ptr<ExprNode> var1 = Variable::Create(m_schema.get(), 0);
        std::unique_ptr<ExprNode> lit1 = Literal::Create(Datum::From((int32_t)100), initoids::TYP_INT4);
        std::unique_ptr<ExprNode> lt = BinaryOperator::Create(OPTYPE_LT, std::move(var1), std::move(lit1));
        std::unique_ptr<ExprNode> var2 = Variable::Create(m_schema.get(), 2);
        std::unique_ptr<ExprNode> lit2 = Literal::Create(Datum::From((double)1.414), initoids::TYP_DOUBLE);
        std::unique_ptr<ExprNode> ge = BinaryOperator::Create(OPTYPE_GE, std::move(var2), std::move(lit2));
        std::unique_ptr<ExprNode> expr = OrOperator::Create(std::move(lt), std::move(ge));
        ASSERT_EQ(expr->Eval(m_record).GetBool(), true);
        ASSERT_EQ(expr->Eval(m_rec_buf.data()).GetBool(), true);
    }

    TDB_TEST_END
}

TEST_F(BasicTestExpression, TestExpressionCast) {
    TDB_TEST_BEGIN

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 4);
        std::unique_ptr<ExprNode> expr = Cast::Create(initoids::TYP_VARCHAR, std::move(var), false);
        ASSERT_EQ(expr->Eval(m_record).GetVarlenAsStringView(), str_literal2);
        ASSERT_EQ(expr->Eval(m_rec_buf.data()).GetVarlenAsStringView(), absl::string_view(str_literal2));
    }

    {
         std::unique_ptr<ExprNode> lit = Literal::Create(Datum::From((double)16.45483), initoids::TYP_DOUBLE);
         std::unique_ptr<ExprNode> expr = Cast::Create(initoids::TYP_UINT8, std::move(lit), true);
         ASSERT_EQ(expr->Eval(m_record).GetUInt64(), 16);
         ASSERT_EQ(expr->Eval(m_rec_buf.data()).GetUInt64(), 16);
    }

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 2);
        std::unique_ptr<ExprNode> expr = Cast::Create(initoids::TYP_UINT8, std::move(var), false);
        ASSERT_EQ(expr->Eval(m_record).GetUInt64(), 3);
    }

    TDB_TEST_END
}

TEST_F(BasicTestExpression, TestExpressionFuncCall) {
    TDB_TEST_BEGIN

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 3);
        std::unique_ptr<ExprNode> lit = Literal::Create(Datum::FromCString("hahaha"), initoids::TYP_VARCHAR);
        auto oid = g_catcache->FindFunctionByName("VARCHAR_concat");
        ASSERT_NE(oid, InvalidOid);
        std::unique_ptr<ExprNode> expr = FuncCallOperator::Create(oid, std::move(var), std::move(lit));
        ASSERT_EQ(expr->Eval(m_record).GetVarlenAsStringView(), "hello world!!hahaha");
    }

    {
        std::unique_ptr<ExprNode> var = Variable::Create(m_schema.get(), 3);
        auto oid = g_catcache->FindFunctionByName("VARCHAR_length");
        ASSERT_NE(oid, InvalidOid);
        std::unique_ptr<ExprNode> expr = FuncCallOperator::Create(oid, std::move(var));
        ASSERT_EQ(expr->Eval(m_record).GetInt16(), 13);
    }

    TDB_TEST_END
}

}
