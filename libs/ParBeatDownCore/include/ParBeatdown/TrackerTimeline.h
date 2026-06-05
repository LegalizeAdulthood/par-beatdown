#pragma once

#include <string>

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

std::string tracker_timeline_schema_shell(const TrackerTimelineSettings &settings = {});

} // namespace par_beatdown
