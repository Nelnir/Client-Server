#include "tst_casename.h"
#include "tst_mockserver.h"

#include <gtest/gtest.h>

TEST(ServerCase, Name)
{
    MockServer serve;
    EXPECT_CALL(serve, run());

    serve.run();
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
