#include <ParBeatdown/Version.h>

#include <gtest/gtest.h>

#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
#include <ParBeatdown/TrackerTimeline.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#endif

TEST(ParBeatDownCore, version)
{
    ASSERT_STREQ("0.1.0", par_beatdown::version());
}

#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
namespace
{

using Json = nlohmann::json;

Json read_json_file(const std::filesystem::path &path)
{
    std::ifstream input{path, std::ios::binary};
    if (!input)
    {
        throw std::runtime_error{"could not open JSON file: " + path.string()};
    }

    Json json;
    input >> json;
    return json;
}

void assert_description(const Json &schema, const std::string &path)
{
    ASSERT_TRUE(schema.contains("description")) << path;
    ASSERT_TRUE(schema.at("description").is_string()) << path;
    ASSERT_FALSE(schema.at("description").get<std::string>().empty()) << path;
}

void check_schema_descriptions(const Json &schema, const std::string &path);

void check_schema_map(const Json &schema, const std::string &field, const std::string &path)
{
    const auto found = schema.find(field);
    if (found == schema.end())
    {
        return;
    }

    ASSERT_TRUE(found->is_object()) << path + "." + field;
    for (const auto &[name, child] : found->items())
    {
        ASSERT_TRUE(child.is_object()) << path + "." + field + "." + name;
        check_schema_descriptions(child, path + "." + field + "." + name);
    }
}

void check_schema_array(const Json &schema, const std::string &field, const std::string &path)
{
    const auto found = schema.find(field);
    if (found == schema.end())
    {
        return;
    }

    ASSERT_TRUE(found->is_array()) << path + "." + field;
    for (std::size_t index = 0; index < found->size(); ++index)
    {
        const auto &child = found->at(index);
        ASSERT_TRUE(child.is_object()) << path + "." + field + "[" + std::to_string(index) + "]";
        check_schema_descriptions(child, path + "." + field + "[" + std::to_string(index) + "]");
    }
}

void check_schema_object(const Json &schema, const std::string &field, const std::string &path)
{
    const auto found = schema.find(field);
    if (found == schema.end() || found->is_boolean())
    {
        return;
    }

    ASSERT_TRUE(found->is_object()) << path + "." + field;
    check_schema_descriptions(*found, path + "." + field);
}

void check_schema_descriptions(const Json &schema, const std::string &path)
{
    assert_description(schema, path);
    check_schema_map(schema, "properties", path);
    check_schema_map(schema, "$defs", path);
    check_schema_object(schema, "items", path);
    check_schema_object(schema, "additionalProperties", path);
    check_schema_array(schema, "allOf", path);
    check_schema_array(schema, "anyOf", path);
    check_schema_array(schema, "oneOf", path);
}

} // namespace

TEST(ParBeatDownCore, trackerLibraryVersion)
{
    ASSERT_NE(0U, par_beatdown::tracker_library_version());
}

TEST(ParBeatDownCore, jsonSchemasHaveDescriptions)
{
    const auto schema_dir = std::filesystem::path{PAR_BEATDOWN_SOURCE_DIR} / "schemas";
    const std::vector<std::string> schema_files{"tracker-timeline.schema.json", "beat-keys.schema.json",
        "beat-keys-overlay.schema.json", "beat-keys-merged-animation.schema.json"};

    for (const auto &schema_file : schema_files)
    {
        const auto path = schema_dir / schema_file;
        const auto schema = read_json_file(path);
        ASSERT_EQ("https://json-schema.org/draft/2020-12/schema", schema.at("$schema").get<std::string>())
            << schema_file;
        ASSERT_TRUE(schema.at("$id").is_string()) << schema_file;
        ASSERT_TRUE(schema.at("title").is_string()) << schema_file;
        check_schema_descriptions(schema, schema_file);
    }
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

TEST(ParBeatDownCore, trackerTimelineBuildsFeatureFrames)
{
    const auto file = std::string{PAR_BEATDOWN_TEST_DATA_DIR} + "/my_neighbors_kid_is_an_internet_addict.xm";
    par_beatdown::TrackerModule module{file};
    par_beatdown::TrackerTimelineSettings settings;
    settings.include_module = false;
    settings.include_features = true;

    const auto features = module.feature_frames(settings);
    ASSERT_FALSE(features.empty());
    ASSERT_EQ(4911U, features.size());
    ASSERT_EQ(0.0, features.front().time_seconds);
    ASSERT_EQ(0, features.front().frame);
    ASSERT_NEAR(0.104656, features.front().rms, 0.000001);
    ASSERT_NEAR(0.255160, features.front().peak, 0.000001);
    ASSERT_EQ(3, features.front().active_channels);
    ASSERT_EQ(static_cast<std::size_t>(module.module_info().channel_count), features.front().channel_vu_mono.size());
    ASSERT_NEAR(1.314777, features.front().channel_vu_mono.at(0), 0.000001);

    const auto json = nlohmann::json::parse(par_beatdown::tracker_timeline_json(module, settings));
    ASSERT_EQ(0, json.at("module").at("channel_count"));
    ASSERT_TRUE(json.at("events").empty());
    ASSERT_FALSE(json.at("features").empty());
    ASSERT_EQ(features.size(), json.at("features").size());
    ASSERT_EQ(0, json.at("features").at(0).at("frame"));
    ASSERT_DOUBLE_EQ(0.104656, json.at("features").at(0).at("rms").get<double>());
    ASSERT_DOUBLE_EQ(0.25516, json.at("features").at(0).at("peak").get<double>());
    ASSERT_TRUE(json.at("features").at(0).contains("channel_vu_mono"));
}

TEST(ParBeatDownCore, trackerTimelineAppliesTimeWindow)
{
    const auto file = std::string{PAR_BEATDOWN_TEST_DATA_DIR} + "/my_neighbors_kid_is_an_internet_addict.xm";
    par_beatdown::TrackerModule module{file};
    par_beatdown::TrackerTimelineSettings settings;
    settings.include_module = false;
    settings.include_timeline = true;
    settings.include_pattern_events = true;
    settings.start_seconds = 1.0;
    settings.duration_seconds = 2.0;

    const auto clock = module.clock_info(settings);
    ASSERT_DOUBLE_EQ(2.0, clock.timeline.duration_seconds);
    ASSERT_EQ(0, clock.timeline.first_frame);
    ASSERT_EQ(60, clock.timeline.last_frame);
    ASSERT_EQ(61, clock.timeline.frames);
    ASSERT_FALSE(clock.events.empty());
    for (const auto &event : clock.events)
    {
        ASSERT_GE(event.time_seconds, 0.0);
        ASSERT_LT(event.time_seconds, 2.0);
        ASSERT_GE(event.frame, 0);
        ASSERT_LE(event.frame, 60);
    }

    const auto pattern_events = module.pattern_events(settings);
    ASSERT_FALSE(pattern_events.empty());
    for (const auto &event : pattern_events)
    {
        ASSERT_GE(event.time_seconds, 0.0);
        ASSERT_LT(event.time_seconds, 2.0);
        ASSERT_GE(event.frame, 0);
        ASSERT_LE(event.frame, 60);
    }

    const auto json = nlohmann::json::parse(par_beatdown::tracker_timeline_json(module, settings));
    ASSERT_DOUBLE_EQ(1.0, json.at("render").at("start_seconds").get<double>());
    ASSERT_DOUBLE_EQ(2.0, json.at("render").at("duration_seconds").get<double>());
    ASSERT_DOUBLE_EQ(2.0, json.at("timeline").at("duration_seconds").get<double>());
    ASSERT_FALSE(json.at("events").empty());
}

TEST(ParBeatDownCore, trackerTimelineAppliesFeatureWindow)
{
    const auto file = std::string{PAR_BEATDOWN_TEST_DATA_DIR} + "/my_neighbors_kid_is_an_internet_addict.xm";
    par_beatdown::TrackerModule module{file};
    par_beatdown::TrackerTimelineSettings settings;
    settings.include_module = false;
    settings.include_features = true;
    settings.start_seconds = 1.0;
    settings.duration_seconds = 2.0;
    settings.feature_hop_seconds = 0.5;

    const auto features = module.feature_frames(settings);
    ASSERT_EQ(4U, features.size());
    ASSERT_DOUBLE_EQ(0.0, features.front().time_seconds);
    ASSERT_EQ(0, features.front().frame);
    ASSERT_DOUBLE_EQ(1.5, features.back().time_seconds);
    ASSERT_EQ(45, features.back().frame);

    const auto json = nlohmann::json::parse(par_beatdown::tracker_timeline_json(module, settings));
    ASSERT_DOUBLE_EQ(1.0, json.at("render").at("start_seconds").get<double>());
    ASSERT_DOUBLE_EQ(2.0, json.at("render").at("duration_seconds").get<double>());
    ASSERT_EQ(features.size(), json.at("features").size());
}
#endif
