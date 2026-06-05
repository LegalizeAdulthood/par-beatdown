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

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

std::string tracker_timeline_schema_shell(const TrackerTimelineSettings &settings = {});
std::string tracker_timeline_json(const TrackerModule &module, const TrackerTimelineSettings &settings = {});
std::string tracker_timeline_from_file(const std::string &file, const TrackerTimelineSettings &settings = {});

} // namespace par_beatdown
