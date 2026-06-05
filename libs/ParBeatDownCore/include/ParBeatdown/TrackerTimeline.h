#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace par_beatdown
{

inline constexpr const char *tracker_timeline_schema = "par-beatdown.tracker-timeline";
inline constexpr int tracker_timeline_schema_version = 1;
inline constexpr const char *tracker_backend = "tracker-file";
inline constexpr const char *tracker_library = "libopenmpt";

struct TrackerTimelineSettings
{
    double fps{30.0};
    int sample_rate{48000};
    int channels{2};
    double offset_seconds{0.0};
    double feature_hop_seconds{1.0 / 30.0};
    bool include_module{true};
    bool include_timeline{false};
};

struct TrackerSourceInfo
{
    std::string file;
    std::uintmax_t size_bytes{0};
    std::string format;
    std::string title;
    double duration_seconds{0.0};
    int selected_subsong{0};
    int subsong_count{0};
    std::vector<std::string> log;
};

struct TrackerMetadataItem
{
    std::string key;
    std::string value;
};

struct TrackerSubsongInfo
{
    int index{0};
    std::string name;
    int restart_order{0};
    int restart_row{0};
};

struct TrackerOrderInfo
{
    int index{0};
    int pattern{0};
    std::string name;
    std::string kind;
};

struct TrackerPatternInfo
{
    int index{0};
    std::string name;
    int row_count{0};
    int rows_per_beat{0};
    int rows_per_measure{0};
};

struct TrackerModuleInfo
{
    int channel_count{0};
    int order_count{0};
    int pattern_count{0};
    int instrument_count{0};
    int sample_count{0};
    std::vector<TrackerMetadataItem> metadata;
    std::vector<TrackerSubsongInfo> subsongs;
    std::vector<TrackerOrderInfo> orders;
    std::vector<TrackerPatternInfo> patterns;
};

struct TrackerTimelineInfo
{
    double duration_seconds{0.0};
    int frames{0};
    int first_frame{0};
    int last_frame{-1};
};

struct TrackerOrderClockInfo
{
    int index{0};
    int pattern{0};
    std::string kind;
    double time_seconds{0.0};
    int frame{0};
};

struct TrackerTimelineEvent
{
    std::string kind;
    double time_seconds{0.0};
    int frame{0};
    int order{0};
    int pattern{0};
    int row{0};
};

struct TrackerClockInfo
{
    TrackerTimelineInfo timeline;
    std::vector<TrackerOrderClockInfo> orders;
    std::vector<TrackerTimelineEvent> events;
};

class TrackerModule
{
public:
    explicit TrackerModule(std::string file);
    TrackerModule(TrackerModule &&other) noexcept;
    TrackerModule &operator=(TrackerModule &&other) noexcept;
    TrackerModule(const TrackerModule &other) = delete;
    TrackerModule &operator=(const TrackerModule &other) = delete;
    ~TrackerModule();

    const TrackerSourceInfo &source() const;
    const TrackerModuleInfo &module_info() const;
    TrackerClockInfo clock_info(const TrackerTimelineSettings &settings);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

std::string tracker_timeline_schema_shell(const TrackerTimelineSettings &settings = {});
std::string tracker_timeline_json(TrackerModule &module, const TrackerTimelineSettings &settings = {});
std::string tracker_timeline_from_file(const std::string &file, const TrackerTimelineSettings &settings = {});

} // namespace par_beatdown
