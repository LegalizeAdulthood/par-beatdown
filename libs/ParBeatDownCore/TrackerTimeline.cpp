#include "ParBeatdown/TrackerTimeline.h"

#include "ParBeatdown/Version.h"

#include <libopenmpt/libopenmpt.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace par_beatdown
{
namespace
{

std::ifstream open_tracker_file(const std::string &file)
{
    std::ifstream stream{file, std::ios::binary};
    if (!stream)
    {
        throw std::runtime_error{"could not open tracker file: " + file};
    }
    return stream;
}

std::vector<std::string> split_log_lines(const std::string &text)
{
    std::vector<std::string> lines;
    std::istringstream input{text};
    std::string line;
    while (std::getline(input, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (!line.empty())
        {
            lines.push_back(line);
        }
    }
    return lines;
}

nlohmann::ordered_json make_source_json(const TrackerSourceInfo *source)
{
    if (source == nullptr)
    {
        return nlohmann::ordered_json{{"file", ""}, {"size_bytes", 0}, {"format", ""}, {"title", ""},
            {"duration_seconds", 0.0}, {"selected_subsong", 0}, {"subsong_count", 0}};
    }

    return nlohmann::ordered_json{{"file", source->file}, {"size_bytes", source->size_bytes},
        {"format", source->format}, {"title", source->title}, {"duration_seconds", source->duration_seconds},
        {"selected_subsong", source->selected_subsong}, {"subsong_count", source->subsong_count}};
}

nlohmann::ordered_json make_diagnostics_json(const TrackerSourceInfo *source)
{
    auto log = nlohmann::ordered_json::array();
    if (source != nullptr)
    {
        for (const auto &line : source->log)
        {
            log.push_back(line);
        }
    }

    return nlohmann::ordered_json{
        {"warnings", nlohmann::ordered_json::array()}, {"unsupported", nlohmann::ordered_json::array()}, {"log", log}};
}

std::string dump_tracker_timeline(const TrackerTimelineSettings &settings, const TrackerSourceInfo *source)
{
    nlohmann::ordered_json root{{"schema", tracker_timeline_schema}, {"version", tracker_timeline_schema_version},
        {"generator",
            nlohmann::ordered_json{{"name", "par-beatdown"}, {"version", version()}, {"backend", tracker_backend},
                {"library", tracker_library}, {"library_version", openmpt::string::get("library_version")}}},
        {"source", make_source_json(source)},
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
        {"diagnostics", make_diagnostics_json(source)}};

    return root.dump(2) + "\n";
}

} // namespace

struct TrackerModule::Impl
{
    explicit Impl(std::string file);

    std::string metadata_value(const std::vector<std::string> &keys, const std::string &key) const;
    void load_source_info();

    std::string file_;
    std::uintmax_t size_bytes_;
    std::ifstream stream_;
    std::ostringstream log_;
    openmpt::module module_;
    TrackerSourceInfo source_;
};

TrackerModule::Impl::Impl(std::string file) :
    file_{std::move(file)},
    size_bytes_{std::filesystem::file_size(file_)},
    stream_{open_tracker_file(file_)},
    module_{stream_, log_}
{
    load_source_info();
}

std::string TrackerModule::Impl::metadata_value(const std::vector<std::string> &keys, const std::string &key) const
{
    return std::find(keys.begin(), keys.end(), key) == keys.end() ? std::string{} : module_.get_metadata(key);
}

void TrackerModule::Impl::load_source_info()
{
    const auto metadata_keys = module_.get_metadata_keys();

    source_.file = file_;
    source_.size_bytes = size_bytes_;
    source_.format = metadata_value(metadata_keys, "type");
    source_.title = metadata_value(metadata_keys, "title");
    source_.duration_seconds = module_.get_duration_seconds();
    source_.selected_subsong = module_.get_selected_subsong();
    source_.subsong_count = module_.get_num_subsongs();
    source_.log = split_log_lines(log_.str());
}

TrackerModule::TrackerModule(std::string file)
try :
    impl_{std::make_unique<Impl>(std::move(file))}
{
}
catch (const openmpt::exception &exception)
{
    throw std::runtime_error{exception.what()};
}

TrackerModule::TrackerModule(TrackerModule &&other) noexcept = default;

TrackerModule &TrackerModule::operator=(TrackerModule &&other) noexcept = default;

TrackerModule::~TrackerModule() = default;

const TrackerSourceInfo &TrackerModule::source() const
{
    return impl_->source_;
}

std::string tracker_timeline_schema_shell(const TrackerTimelineSettings &settings)
{
    return dump_tracker_timeline(settings, nullptr);
}

std::string tracker_timeline_json(const TrackerModule &module, const TrackerTimelineSettings &settings)
{
    return dump_tracker_timeline(settings, &module.source());
}

std::string tracker_timeline_from_file(const std::string &file, const TrackerTimelineSettings &settings)
{
    const TrackerModule module{file};
    return tracker_timeline_json(module, settings);
}

} // namespace par_beatdown
