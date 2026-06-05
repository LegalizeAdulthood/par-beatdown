#include <ParBeatDownCore/version.h>

#include <gtest/gtest.h>

TEST(ParBeatDownCore, version)
{
    ASSERT_STREQ("0.1.0", ParBeatDownCore::version());
}
