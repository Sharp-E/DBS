#include "base/TDBDBTest.h"
#include "dbmain/Database.h"

namespace taco {

class BasicTestRepoCompilesAndRuns: public TDBDBTest {
public:
    void
    SetUp() override {
        g_test_no_catcache = true;
        TDBDBTest::SetUp();
    }
};

TEST_F(BasicTestRepoCompilesAndRuns, TestShouldAlwaysSucceed) {
    SUCCEED();
}

}   // namespace taco
