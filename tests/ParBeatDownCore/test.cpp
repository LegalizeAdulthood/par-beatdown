#include <ParBeatdown/Version.h>

#include <gtest/gtest.h>

#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
#include <ParBeatdown/TrackerTimeline.h>

#include <nlohmann/json.hpp>
#endif

TEST(ParBeatDownCore, version)
{
    ASSERT_STREQ("0.1.0", par_beatdown::version());
}

#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
TEST(ParBeatDownCore, trackerLibraryVersion)
{
    ASSERT_NE(0U, par_beatdown::tracker_library_version());
}

TEST(ParBeatDownCore, trackerTimelineSchemaShell)
{
    const auto text = par_beatdown::tracker_timeline_schema_shell();
    const auto json = nlohmann::json::parse(text);

    ASSERT_EQ('\n', text.back());
    ASSERT_EQ(par_beatdown::tracker_timeline_schema, json.at("schema"));
    ASSERT_EQ(par_beatdown::tracker_timeline_schema_version, json.at("version"));
    ASSERT_EQ("par-beatdown", json.at("generator").at("name"));
    ASSERT_EQ(par_beatdown::tracker_backend, json.at("generator").at("backend"));
    ASSERT_EQ(par_beatdown::tracker_library, json.at("generator").at("library"));
    ASSERT_TRUE(json.at("events").empty());
    ASSERT_TRUE(json.at("features").empty());
    ASSERT_TRUE(json.at("diagnostics").at("warnings").empty());
}
#endif
