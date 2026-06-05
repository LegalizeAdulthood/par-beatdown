#include "ParBeatdown/TrackerTimeline.h"

#include "ParBeatdown/Version.h"

#include <libopenmpt/libopenmpt.hpp>
#include <nlohmann/json.hpp>

#include <string>

namespace par_beatdown
{

std::string tracker_timeline_schema_shell(const TrackerTimelineSettings &settings)
{
    nlohmann::ordered_json root{{"schema", tracker_timeline_schema}, {"version", tracker_timeline_schema_version},
        {"generator",
            nlohmann::ordered_json{{"name", "par-beatdown"}, {"version", version()}, {"backend", tracker_backend},
                {"library", tracker_library}, {"library_version", openmpt::string::get("library_version")}}},
        {"source",
            nlohmann::ordered_json{{"file", ""}, {"size_bytes", 0}, {"format", ""}, {"title", ""},
                {"duration_seconds", 0.0}, {"selected_subsong", 0}, {"subsong_count", 0}}},
        {"render",
            nlohmann::ordered_json{{"fps", settings.fps}, {"sample_rate", settings.sample_rate},
                {"channels", settings.channels}, {"offset_seconds", settings.offset_seconds},
                {"feature_hop_seconds", settings.feature_hop_seconds}}},
        {"module",
            nlohmann::ordered_json{{"channel_count", 0}, {"order_count", 0}, {"pattern_count", 0},
                {"instrument_count", 0}, {"sample_count", 0}, {"metadata", nlohmann::ordered_json::array()},
                {"subsongs", nlohmann::ordered_json::array()}, {"orders", nlohmann::ordered_json::array()},
                {"patterns", nlohmann::ordered_json::array()}}},
        {"timeline",
            nlohmann::ordered_json{{"duration_seconds", 0.0}, {"frames", 0}, {"first_frame", 0}, {"last_frame", -1}}},
        {"events", nlohmann::ordered_json::array()}, {"features", nlohmann::ordered_json::array()},
        {"diagnostics",
            nlohmann::ordered_json{{"warnings", nlohmann::ordered_json::array()},
                {"unsupported", nlohmann::ordered_json::array()}, {"log", nlohmann::ordered_json::array()}}}};

    return root.dump(2) + "\n";
}

} // namespace par_beatdown
