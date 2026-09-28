#include "base/TDBNonDBTest.h"

#include "catalog/BootstrapCatCache.h"
#include "catalog/Schema.h"
#include "catalog/systables.h"
#include "utils/builtin_funcs.h"

namespace taco {

class TestSchema: public TDBNonDBTest {
public:
    void
    SetUp() override {
        TDBNonDBTest::SetUp();
        TDB_TEST_BEGIN

        m_catcache = std::make_unique<BootstrapCatCache>();
        ASSERT_NO_ERROR(m_catcache->Init());

        // A few checks to make sure the initial built-in types match our
        // expection.

        // See BUILDIR/generated_source/catalog/systables/bootstrap_data.cpp
        // for available types in the BootstrapCatCache.
        const SysTable_Type *typ = m_catcache->FindType(initoids::TYP_INT2);
        ASSERT_NE(typ, nullptr);
        ASSERT_EQ(typ->typlen(), 2);
        ASSERT_FALSE(typ->typisvarlen());
        ASSERT_FALSE(typ->typbyref());
        ASSERT_EQ(typ->typalign(), 2u);

        typ = m_catcache->FindType(initoids::TYP_UINT1);
        ASSERT_NE(typ, nullptr);
        ASSERT_EQ(typ->typlen(), 1);
        ASSERT_FALSE(typ->typisvarlen());
        ASSERT_FALSE(typ->typbyref());
        ASSERT_EQ(typ->typalign(), 1u);

        typ = m_catcache->FindType(initoids::TYP_UINT4);
        ASSERT_NE(typ, nullptr);
        ASSERT_EQ(typ->typlen(), 4);
        ASSERT_FALSE(typ->typisvarlen());
        ASSERT_FALSE(typ->typbyref());
        ASSERT_EQ(typ->typalign(), 4u);

        typ = m_catcache->FindType(initoids::TYP_UINT8);
        ASSERT_NE(typ, nullptr);
        ASSERT_EQ(typ->typlen(), 8);
        ASSERT_FALSE(typ->typisvarlen());
        ASSERT_FALSE(typ->typbyref());
        ASSERT_EQ(typ->typalign(), 8u);

        typ = m_catcache->FindType(initoids::TYP_VARCHAR);
        ASSERT_NE(typ, nullptr);
        ASSERT_TRUE(typ->typisvarlen());
        ASSERT_TRUE(typ->typbyref());
        // varlen types have -1 specified in Type catalog
        ASSERT_LT(typ->typlen(), 0);
        ASSERT_NE(typ->typinfunc(), InvalidOid);
        m_varcharin = FindBuiltinFunction(typ->typinfunc());
        ASSERT_TRUE(m_varcharin);

        typ = m_catcache->FindType(initoids::TYP_CHAR);
        ASSERT_NE(typ, nullptr);
        ASSERT_FALSE(typ->typisvarlen());
        ASSERT_TRUE(typ->typbyref());
        ASSERT_NE(typ->typlenfunc(), InvalidOid);
        ASSERT_NE(typ->typinfunc(), InvalidOid);
        m_charin = FindBuiltinFunction(typ->typinfunc());
        ASSERT_TRUE(m_charin);

        typ = m_catcache->FindType(initoids::TYP_DECIMAL);
        ASSERT_NE(typ, nullptr);
        ASSERT_TRUE(typ->typisvarlen());
        ASSERT_TRUE(typ->typbyref());
        ASSERT_LT(typ->typlen(), 0);
        ASSERT_NE(typ->typinfunc(), InvalidOid);
        m_decimalin = FindBuiltinFunction(typ->typinfunc());
        ASSERT_TRUE(m_decimalin);
        m_decimalout = FindBuiltinFunction(typ->typoutfunc());
        ASSERT_TRUE(m_decimalout);

        TDB_TEST_END
    }

    std::unique_ptr<Schema>
    GetSchemaA() {
        std::unique_ptr<Schema> s(Schema::Create(
            /*typid=*/{
                /*0:*/initoids::TYP_INT2,
                /*1:*/initoids::TYP_UINT8,
                /*2:*/initoids::TYP_UINT1,
                /*3:*/initoids::TYP_UINT4
            },
            /*typparam=*/{0, 0, 0, 0},
            /*nullable=*/{false, false, false, false}
        ));
        // Must use BootstrapCatCache here as we don't have a properly
        // initialized database instance in this test.
        //
        // For the rest of the projects, please make sure to use the
        // no-argument version of ComputeLayout(), which uses the properly
        // initialized global catalog.
        s->ComputeLayout(m_catcache.get());
        return s;
    }

    std::unique_ptr<Schema>
    GetSchemaB(uint64_t charlen1, uint64_t charlen3) {
        std::unique_ptr<Schema> s(Schema::Create(
            /*typid=*/{
                /*0:*/initoids::TYP_INT2,
                /*1:*/initoids::TYP_CHAR,
                /*2:*/initoids::TYP_UINT4,
                /*3:*/initoids::TYP_CHAR
            },
            /*typparam=*/{0, charlen1, 0, charlen3},
            /*nullable=*/{false, false, false, false}
        ));
        s->ComputeLayout(m_catcache.get());
        return s;

    }

    std::unique_ptr<Schema>
    GetSchemaC() {
        std::unique_ptr<Schema> s(Schema::Create(
            /*typid=*/{
                /*0:*/initoids::TYP_INT2,
                /*1:*/initoids::TYP_VARCHAR,
                /*2:*/initoids::TYP_UINT1,
                /*3:*/initoids::TYP_DECIMAL
            },
            /*typparam=*/{0, 0, 0, 0},
            /*nullable=*/{false, false, false, false}
        ));
        s->ComputeLayout(m_catcache.get());
        return s;
    }

    std::unique_ptr<Schema>
    GetSchemaD() {
        std::unique_ptr<Schema> s(Schema::Create(
            /*typid=*/{
                /*0:*/initoids::TYP_UINT4,
                /*1:*/initoids::TYP_DECIMAL,
                /*2:*/initoids::TYP_INT2,
                /*3:*/initoids::TYP_VARCHAR
            },
            /*typparam=*/{0, 0, 0, 0},
            /*nullable=*/{false, false, false, false}
        ));
        s->ComputeLayout(m_catcache.get());
        return s;
    }

    std::unique_ptr<Schema>
    GetSchemaAWithNullable() {
        std::unique_ptr<Schema> s(Schema::Create(
            /*typid=*/{
                /*0:*/initoids::TYP_INT2,
                /*1:*/initoids::TYP_UINT8,
                /*2:*/initoids::TYP_UINT1,
                /*3:*/initoids::TYP_UINT4
            },
            /*typparam=*/{0, 0, 0, 0},
            /*nullable=*/{true, false, false, true}
        ));
        // Must use BootstrapCatCache here as we don't have a properly
        // initialized database instance in this test.
        //
        // For the rest of the projects, please make sure to use the
        // no-argument version of ComputeLayout(), which uses the properly
        // initialized global catalog.
        s->ComputeLayout(m_catcache.get());
        return s;
    }

    std::unique_ptr<Schema>
    GetSchemaDWithNullable() {
        std::unique_ptr<Schema> s(Schema::Create(
            /*typid=*/{
                /*0:*/initoids::TYP_UINT4,
                /*1:*/initoids::TYP_DECIMAL,
                /*2:*/initoids::TYP_INT2,
                /*3:*/initoids::TYP_VARCHAR
            },
            /*typparam=*/{0, 0, 0, 0},
            /*nullable=*/{false, true, true, false}
        ));
        s->ComputeLayout(m_catcache.get());
        return s;
    }


    Datum
    Char(const char *in, uint64_t charlen) {
        return FunctionCallWithTypparam(m_charin, charlen,
            Datum::FromCString(in));
    }

    /*
     * This function does not copy the underlying string object.
     */
    Datum
    Varchar(absl::string_view in) {
        return Datum::FromVarlenAsStringView(in);
    }

    Datum
    Decimal(absl::string_view in) {
        return FunctionCall(m_decimalin,
                            Datum::FromVarlenAsStringView(in));
    }

    Datum
    DecimalToString(DatumRef d) {
        return FunctionCall(m_decimalout, d);
    }

private:
    std::unique_ptr<BootstrapCatCache> m_catcache;
    FunctionInfo m_varcharin;
    FunctionInfo m_charin;
    FunctionInfo m_decimalin;
    FunctionInfo m_decimalout;
};

}    // namespace taco

