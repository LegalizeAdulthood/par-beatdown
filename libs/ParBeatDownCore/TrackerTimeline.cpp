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

std::string list_value(const std::vector<std::string> &items, int index)
{
    return index >= 0 && static_cast<std::size_t>(index) < items.size() ? items[static_cast<std::size_t>(index)]
                                                                        : std::string{};
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

nlohmann::ordered_json make_module_json(const TrackerModuleInfo *module)
{
    if (module == nullptr)
    {
        return nlohmann::ordered_json{{"channel_count", 0}, {"order_count", 0}, {"pattern_count", 0},
            {"instrument_count", 0}, {"sample_count", 0}, {"metadata", nlohmann::ordered_json::array()},
            {"subsongs", nlohmann::ordered_json::array()}, {"orders", nlohmann::ordered_json::array()},
            {"patterns", nlohmann::ordered_json::array()}};
    }

    auto metadata = nlohmann::ordered_json::array();
    for (const auto &item : module->metadata)
    {
        metadata.push_back(nlohmann::ordered_json{{"key", item.key}, {"value", item.value}});
    }

    auto subsongs = nlohmann::ordered_json::array();
    for (const auto &subsong : module->subsongs)
    {
        subsongs.push_back(nlohmann::ordered_json{{"index", subsong.index}, {"name", subsong.name},
            {"restart_order", subsong.restart_order}, {"restart_row", subsong.restart_row}});
    }

    auto orders = nlohmann::ordered_json::array();
    for (const auto &order : module->orders)
    {
        orders.push_back(nlohmann::ordered_json{
            {"index", order.index}, {"pattern", order.pattern}, {"name", order.name}, {"kind", order.kind}});
    }

    auto patterns = nlohmann::ordered_json::array();
    for (const auto &pattern : module->patterns)
    {
        patterns.push_back(
            nlohmann::ordered_json{{"index", pattern.index}, {"name", pattern.name}, {"row_count", pattern.row_count},
                {"rows_per_beat", pattern.rows_per_beat}, {"rows_per_measure", pattern.rows_per_measure}});
    }

    return nlohmann::ordered_json{{"channel_count", module->channel_count}, {"order_count", module->order_count},
        {"pattern_count", module->pattern_count}, {"instrument_count", module->instrument_count},
        {"sample_count", module->sample_count}, {"metadata", metadata}, {"subsongs", subsongs}, {"orders", orders},
        {"patterns", patterns}};
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

std::string dump_tracker_timeline(
    const TrackerTimelineSettings &settings, const TrackerSourceInfo *source, const TrackerModuleInfo *module)
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
        {"module", make_module_json(module)},
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
    std::string order_kind(int order) const;
    void load_source_info();
    void load_module_info();

    std::string file_;
    std::uintmax_t size_bytes_;
    std::ifstream stream_;
    std::ostringstream log_;
    openmpt::module module_;
    TrackerSourceInfo source_;
    TrackerModuleInfo module_info_;
};

TrackerModule::Impl::Impl(std::string file) :
    file_{std::move(file)},
    size_bytes_{std::filesystem::file_size(file_)},
    stream_{open_tracker_file(file_)},
    module_{stream_, log_}
{
    load_source_info();
    load_module_info();
}

std::string TrackerModule::Impl::metadata_value(const std::vector<std::string> &keys, const std::string &key) const
{
    return std::find(keys.begin(), keys.end(), key) == keys.end() ? std::string{} : module_.get_metadata(key);
}

std::string TrackerModule::Impl::order_kind(int order) const
{
    if (module_.is_order_skip_entry(order))
    {
        return "skip";
    }
    if (module_.is_order_stop_entry(order))
    {
        return "stop";
    }
    return "pattern";
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

void TrackerModule::Impl::load_module_info()
{
    const auto metadata_keys = module_.get_metadata_keys();
    const auto subsong_names = module_.get_subsong_names();
    const auto order_names = module_.get_order_names();
    const auto pattern_names = module_.get_pattern_names();

    module_info_.channel_count = module_.get_num_channels();
    module_info_.order_count = module_.get_num_orders();
    module_info_.pattern_count = module_.get_num_patterns();
    module_info_.instrument_count = module_.get_num_instruments();
    module_info_.sample_count = module_.get_num_samples();

    for (const auto &key : metadata_keys)
    {
        module_info_.metadata.push_back(TrackerMetadataItem{key, module_.get_metadata(key)});
    }

    for (int index = 0; index < source_.subsong_count; ++index)
    {
        module_info_.subsongs.push_back(TrackerSubsongInfo{
            index, list_value(subsong_names, index), module_.get_restart_order(index), module_.get_restart_row(index)});
    }

    for (int index = 0; index < module_info_.order_count; ++index)
    {
        module_info_.orders.push_back(TrackerOrderInfo{
            index, module_.get_order_pattern(index), list_value(order_names, index), order_kind(index)});
    }

    for (int index = 0; index < module_info_.pattern_count; ++index)
    {
        module_info_.patterns.push_back(
            TrackerPatternInfo{index, list_value(pattern_names, index), module_.get_pattern_num_rows(index),
                module_.get_pattern_rows_per_beat(index), module_.get_pattern_rows_per_measure(index)});
    }
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

const TrackerModuleInfo &TrackerModule::module_info() const
{
    return impl_->module_info_;
}

std::string tracker_timeline_schema_shell(const TrackerTimelineSettings &settings)
{
    return dump_tracker_timeline(settings, nullptr, nullptr);
}

std::string tracker_timeline_json(const TrackerModule &module, const TrackerTimelineSettings &settings)
{
    return dump_tracker_timeline(settings, &module.source(), settings.include_module ? &module.module_info() : nullptr);
}

std::string tracker_timeline_from_file(const std::string &file, const TrackerTimelineSettings &settings)
{
    const TrackerModule module{file};
    return tracker_timeline_json(module, settings);
}

} // namespace par_beatdown
