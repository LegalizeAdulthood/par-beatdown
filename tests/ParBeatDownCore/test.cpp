#include <ParBeatdown/Version.h>

#include <gtest/gtest.h>

#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
#include <ParBeatdown/TrackerTimeline.h>

#include <algorithm>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
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

TEST(ParBeatDownCore, trackerModuleLoadsSource)
{
    const auto file = std::string{PAR_BEATDOWN_TEST_DATA_DIR} + "/my_neighbors_kid_is_an_internet_addict.xm";
    par_beatdown::TrackerModule module{file};
    const auto &source = module.source();

    ASSERT_EQ(file, source.file);
    ASSERT_EQ(static_cast<std::uintmax_t>(6414784), source.size_bytes);
    ASSERT_EQ("xm", source.format);
    ASSERT_GT(source.duration_seconds, 0.0);
    ASSERT_GE(source.selected_subsong, -1);
    ASSERT_GE(source.subsong_count, 1);

    const auto json = nlohmann::json::parse(par_beatdown::tracker_timeline_json(module));
    ASSERT_EQ(source.file, json.at("source").at("file"));
    ASSERT_EQ(source.size_bytes, json.at("source").at("size_bytes").get<std::uintmax_t>());
    ASSERT_EQ(source.format, json.at("source").at("format"));
    ASSERT_EQ(source.title, json.at("source").at("title"));
    ASSERT_DOUBLE_EQ(source.duration_seconds, json.at("source").at("duration_seconds").get<double>());
    ASSERT_EQ(source.selected_subsong, json.at("source").at("selected_subsong"));
    ASSERT_EQ(source.subsong_count, json.at("source").at("subsong_count"));
    ASSERT_EQ(source.log.size(), json.at("diagnostics").at("log").size());
}

TEST(ParBeatDownCore, trackerModuleLoadsStaticStructure)
{
    const auto file = std::string{PAR_BEATDOWN_TEST_DATA_DIR} + "/my_neighbors_kid_is_an_internet_addict.xm";
    par_beatdown::TrackerModule module{file};
    const auto &source = module.source();
    const auto &info = module.module_info();

    ASSERT_GT(info.channel_count, 0);
    ASSERT_GT(info.order_count, 0);
    ASSERT_GT(info.pattern_count, 0);
    ASSERT_GT(info.sample_count, 0);
    ASSERT_FALSE(info.metadata.empty());
    ASSERT_EQ(static_cast<std::size_t>(source.subsong_count), info.subsongs.size());
    ASSERT_EQ(static_cast<std::size_t>(info.order_count), info.orders.size());
    ASSERT_EQ(static_cast<std::size_t>(info.pattern_count), info.patterns.size());

    const auto json = nlohmann::json::parse(par_beatdown::tracker_timeline_json(module));
    ASSERT_EQ(info.channel_count, json.at("module").at("channel_count"));
    ASSERT_EQ(info.order_count, json.at("module").at("order_count"));
    ASSERT_EQ(info.pattern_count, json.at("module").at("pattern_count"));
    ASSERT_EQ(info.instrument_count, json.at("module").at("instrument_count"));
    ASSERT_EQ(info.sample_count, json.at("module").at("sample_count"));
    ASSERT_EQ(info.metadata.size(), json.at("module").at("metadata").size());
    ASSERT_EQ(info.subsongs.size(), json.at("module").at("subsongs").size());
    ASSERT_EQ(info.orders.size(), json.at("module").at("orders").size());
    ASSERT_EQ(info.patterns.size(), json.at("module").at("patterns").size());
    ASSERT_EQ("pattern", json.at("module").at("orders").at(0).at("kind"));
    ASSERT_GT(json.at("module").at("patterns").at(0).at("row_count").get<int>(), 0);
}

TEST(ParBeatDownCore, trackerTimelineCanOmitOptionalModule)
{
    const auto file = std::string{PAR_BEATDOWN_TEST_DATA_DIR} + "/my_neighbors_kid_is_an_internet_addict.xm";
    par_beatdown::TrackerModule module{file};
    par_beatdown::TrackerTimelineSettings settings;
    settings.include_module = false;

    const auto json = nlohmann::json::parse(par_beatdown::tracker_timeline_json(module, settings));

    ASSERT_EQ(file, json.at("source").at("file"));
    ASSERT_EQ(0, json.at("module").at("channel_count"));
    ASSERT_TRUE(json.at("module").at("metadata").empty());
    ASSERT_TRUE(json.at("module").at("orders").empty());
    ASSERT_TRUE(json.at("module").at("patterns").empty());
}

TEST(ParBeatDownCore, trackerTimelineBuildsClock)
{
    const auto file = std::string{PAR_BEATDOWN_TEST_DATA_DIR} + "/my_neighbors_kid_is_an_internet_addict.xm";
    par_beatdown::TrackerModule module{file};
    par_beatdown::TrackerTimelineSettings settings;
    settings.include_module = false;
    settings.include_timeline = true;

    const auto clock = module.clock_info(settings);
    ASSERT_GT(clock.timeline.duration_seconds, 0.0);
    ASSERT_EQ(0, clock.timeline.first_frame);
    ASSERT_GT(clock.timeline.last_frame, clock.timeline.first_frame);
    ASSERT_EQ(clock.timeline.last_frame + 1, clock.timeline.frames);
    ASSERT_FALSE(clock.orders.empty());
    ASSERT_FALSE(clock.events.empty());

    const auto json = nlohmann::json::parse(par_beatdown::tracker_timeline_json(module, settings));
    ASSERT_EQ(clock.timeline.frames, json.at("timeline").at("frames"));
    ASSERT_FALSE(json.at("module").at("orders").empty());
    ASSERT_TRUE(json.at("module").at("orders").at(0).contains("time_seconds"));
    ASSERT_TRUE(json.at("module").at("orders").at(0).contains("frame"));
    ASSERT_EQ("order", json.at("events").at(0).at("kind"));
    ASSERT_EQ("pattern", json.at("events").at(1).at("kind"));
    ASSERT_EQ("row", json.at("events").at(2).at("kind"));
}

TEST(ParBeatDownCore, trackerTimelineBuildsPatternEvents)
{
    const auto file = std::string{PAR_BEATDOWN_TEST_DATA_DIR} + "/my_neighbors_kid_is_an_internet_addict.xm";
    par_beatdown::TrackerModule module{file};
    par_beatdown::TrackerTimelineSettings settings;
    settings.include_module = false;
    settings.include_pattern_events = true;

    const auto events = module.pattern_events(settings);
    const auto note = std::find_if(events.begin(), events.end(),
        [](const par_beatdown::TrackerTimelineEvent &event) { return event.kind == "note"; });
    const auto effect = std::find_if(events.begin(), events.end(),
        [](const par_beatdown::TrackerTimelineEvent &event) { return event.kind == "effect"; });
    ASSERT_NE(events.end(), note);
    ASSERT_NE(events.end(), effect);
    ASSERT_GE(note->channel, 0);
    ASSERT_FALSE(note->text.empty());
    ASSERT_EQ("C-5", note->note);
    ASSERT_EQ(1, note->instrument);
    ASSERT_EQ("C-5 01 .. ...", note->text);
    ASSERT_GE(effect->channel, 0);
    ASSERT_FALSE(effect->text.empty());
    ASSERT_EQ("7", effect->effect);
    ASSERT_EQ(135, effect->parameter);
    ASSERT_EQ("F#4 04 .. 787", effect->text);

    const auto json = nlohmann::json::parse(par_beatdown::tracker_timeline_json(module, settings));
    ASSERT_EQ(0, json.at("module").at("channel_count"));
    ASSERT_EQ(0, json.at("timeline").at("frames"));
    ASSERT_FALSE(json.at("events").empty());
    ASSERT_TRUE(json.at("events").at(0).contains("channel"));
    ASSERT_TRUE(json.at("events").at(0).contains("text"));
}
#endif
