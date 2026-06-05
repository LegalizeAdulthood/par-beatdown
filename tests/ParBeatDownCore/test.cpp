#include <ParBeatDownCore/version.h>

#include <gtest/gtest.h>

TEST(ParBeatDownCore, version)
{
    ASSERT_STREQ("0.1.0", ParBeatDownCore::version());
}

#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
TEST(ParBeatDownCore, trackerLibraryVersion)
{
    ASSERT_NE(0U, ParBeatDownCore::tracker_library_version());
}
#endif
