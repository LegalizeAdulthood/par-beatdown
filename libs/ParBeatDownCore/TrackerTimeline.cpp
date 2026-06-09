#include "ParBeatdown/TrackerTimeline.h"

#include "ParBeatdown/Version.h"

#include <libopenmpt/libopenmpt.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
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

std::uintmax_t tracker_file_size(const std::string &file)
{
    std::error_code error;
    const auto size = std::filesystem::file_size(file, error);
    return error ? 0 : size;
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

bool is_valid_time(double seconds)
{
    return std::isfinite(seconds) && seconds >= 0.0;
}

bool has_duration_limit(const TrackerTimelineSettings &settings)
{
    return settings.duration_seconds > 0.0;
}

double window_end_seconds(double source_duration, const TrackerTimelineSettings &settings)
{
    if (!is_valid_time(source_duration) || source_duration < settings.start_seconds)
    {
        return settings.start_seconds;
    }
    if (!has_duration_limit(settings))
    {
        return source_duration;
    }
    return std::min(source_duration, settings.start_seconds + settings.duration_seconds);
}

bool is_in_window(double source_seconds, double end_seconds, const TrackerTimelineSettings &settings)
{
    if (source_seconds < settings.start_seconds)
    {
        return false;
    }
    return !has_duration_limit(settings) || source_seconds < end_seconds;
}

double output_seconds_at(double source_seconds, const TrackerTimelineSettings &settings)
{
    return source_seconds - settings.start_seconds;
}

int frame_at_output(double output_seconds, const TrackerTimelineSettings &settings)
{
    return static_cast<int>(std::llround((output_seconds + settings.offset_seconds) * settings.fps));
}

double round_json_number(double value)
{
    return std::round(value * 1000000.0) / 1000000.0;
}

std::string trim_pattern_text(std::string text)
{
    while (!text.empty() && text.front() == ' ')
    {
        text.erase(text.begin());
    }
    while (!text.empty() && text.back() == ' ')
    {
        text.pop_back();
    }
    return text;
}

bool has_pattern_text(const std::string &text)
{
    return std::any_of(text.begin(), text.end(), [](char item) { return item != ' ' && item != '.'; });
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

nlohmann::ordered_json make_order_json(const TrackerOrderInfo &order, const TrackerOrderClockInfo *clock)
{
    auto output = nlohmann::ordered_json{
        {"index", order.index}, {"pattern", order.pattern}, {"name", order.name}, {"kind", order.kind}};
    if (clock != nullptr)
    {
        output["time_seconds"] = clock->time_seconds;
        output["frame"] = clock->frame;
    }
    return output;
}

nlohmann::ordered_json make_order_json(const TrackerOrderClockInfo &clock)
{
    return nlohmann::ordered_json{{"index", clock.index}, {"pattern", clock.pattern}, {"name", ""},
        {"kind", clock.kind}, {"time_seconds", clock.time_seconds}, {"frame", clock.frame}};
}

const TrackerOrderClockInfo *find_order_clock(const TrackerClockInfo *clock, int order)
{
    if (clock == nullptr)
    {
        return nullptr;
    }

    const auto found = std::find_if(clock->orders.begin(), clock->orders.end(),
        [order](const TrackerOrderClockInfo &item) { return item.index == order; });
    return found == clock->orders.end() ? nullptr : &*found;
}

nlohmann::ordered_json make_module_json(const TrackerModuleInfo *module, const TrackerClockInfo *clock)
{
    if (module == nullptr && clock == nullptr)
    {
        return nlohmann::ordered_json{{"channel_count", 0}, {"order_count", 0}, {"pattern_count", 0},
            {"instrument_count", 0}, {"sample_count", 0}, {"metadata", nlohmann::ordered_json::array()},
            {"subsongs", nlohmann::ordered_json::array()}, {"orders", nlohmann::ordered_json::array()},
            {"patterns", nlohmann::ordered_json::array()}};
    }

    auto metadata = nlohmann::ordered_json::array();
    if (module != nullptr)
    {
        for (const auto &item : module->metadata)
        {
            metadata.push_back(nlohmann::ordered_json{{"key", item.key}, {"value", item.value}});
        }
    }

    auto subsongs = nlohmann::ordered_json::array();
    if (module != nullptr)
    {
        for (const auto &subsong : module->subsongs)
        {
            subsongs.push_back(nlohmann::ordered_json{{"index", subsong.index}, {"name", subsong.name},
                {"restart_order", subsong.restart_order}, {"restart_row", subsong.restart_row}});
        }
    }

    auto orders = nlohmann::ordered_json::array();
    if (module != nullptr)
    {
        for (const auto &order : module->orders)
        {
            orders.push_back(make_order_json(order, find_order_clock(clock, order.index)));
        }
    }
    else if (clock != nullptr)
    {
        for (const auto &order : clock->orders)
        {
            orders.push_back(make_order_json(order));
        }
    }

    auto patterns = nlohmann::ordered_json::array();
    if (module != nullptr)
    {
        for (const auto &pattern : module->patterns)
        {
            patterns.push_back(nlohmann::ordered_json{{"index", pattern.index}, {"name", pattern.name},
                {"row_count", pattern.row_count}, {"rows_per_beat", pattern.rows_per_beat},
                {"rows_per_measure", pattern.rows_per_measure}});
        }
    }

    return nlohmann::ordered_json{{"channel_count", module == nullptr ? 0 : module->channel_count},
        {"order_count", module == nullptr ? static_cast<int>(orders.size()) : module->order_count},
        {"pattern_count", module == nullptr ? 0 : module->pattern_count},
        {"instrument_count", module == nullptr ? 0 : module->instrument_count},
        {"sample_count", module == nullptr ? 0 : module->sample_count}, {"metadata", metadata}, {"subsongs", subsongs},
        {"orders", orders}, {"patterns", patterns}};
}

nlohmann::ordered_json make_timeline_json(const TrackerClockInfo *clock)
{
    if (clock == nullptr)
    {
        return nlohmann::ordered_json{{"duration_seconds", 0.0}, {"frames", 0}, {"first_frame", 0}, {"last_frame", -1}};
    }

    return nlohmann::ordered_json{{"duration_seconds", clock->timeline.duration_seconds},
        {"frames", clock->timeline.frames}, {"first_frame", clock->timeline.first_frame},
        {"last_frame", clock->timeline.last_frame}};
}

nlohmann::ordered_json make_event_json(const TrackerTimelineEvent &event)
{
    auto output = nlohmann::ordered_json{{"kind", event.kind}, {"time_seconds", event.time_seconds},
        {"frame", event.frame}, {"order", event.order}, {"pattern", event.pattern}, {"row", event.row}};
    if (event.channel >= 0)
    {
        output["channel"] = event.channel;
    }
    if (!event.note.empty())
    {
        output["note"] = event.note;
    }
    if (event.instrument >= 0)
    {
        output["instrument"] = event.instrument;
    }
    if (event.sample >= 0)
    {
        output["sample"] = event.sample;
    }
    if (event.volume >= 0)
    {
        output["volume"] = event.volume;
    }
    if (!event.effect.empty())
    {
        output["effect"] = event.effect;
    }
    if (event.parameter >= 0)
    {
        output["parameter"] = event.parameter;
    }
    if (!event.text.empty())
    {
        output["text"] = event.text;
    }
    return output;
}

nlohmann::ordered_json make_events_json(const std::vector<TrackerTimelineEvent> *source_events)
{
    auto events = nlohmann::ordered_json::array();
    if (source_events == nullptr)
    {
        return events;
    }

    for (const auto &event : *source_events)
    {
        events.push_back(make_event_json(event));
    }
    return events;
}

nlohmann::ordered_json make_features_json(const std::vector<TrackerFeatureFrame> *source_features)
{
    auto features = nlohmann::ordered_json::array();
    if (source_features == nullptr)
    {
        return features;
    }

    for (const auto &feature : *source_features)
    {
        auto channel_vu_mono = nlohmann::ordered_json::array();
        for (const auto value : feature.channel_vu_mono)
        {
            channel_vu_mono.push_back(round_json_number(value));
        }
        features.push_back(
            nlohmann::ordered_json{{"time_seconds", round_json_number(feature.time_seconds)}, {"frame", feature.frame},
                {"rms", round_json_number(feature.rms)}, {"peak", round_json_number(feature.peak)},
                {"active_channels", feature.active_channels}, {"channel_vu_mono", channel_vu_mono}});
    }
    return features;
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

nlohmann::ordered_json make_render_json(const TrackerTimelineSettings &settings)
{
    auto render = nlohmann::ordered_json{{"fps", settings.fps}, {"sample_rate", settings.sample_rate},
        {"channels", settings.channels}, {"offset_seconds", settings.offset_seconds},
        {"feature_hop_seconds", settings.feature_hop_seconds}};
    if (settings.start_seconds != 0.0 || has_duration_limit(settings))
    {
        render["start_seconds"] = settings.start_seconds;
    }
    if (has_duration_limit(settings))
    {
        render["duration_seconds"] = settings.duration_seconds;
    }
    return render;
}

std::string dump_tracker_timeline(const TrackerTimelineSettings &settings, const TrackerSourceInfo *source,
    const TrackerModuleInfo *module, const TrackerClockInfo *clock, const std::vector<TrackerTimelineEvent> *events,
    const std::vector<TrackerFeatureFrame> *features)
{
    nlohmann::ordered_json root{{"schema", tracker_timeline_schema}, {"version", tracker_timeline_schema_version},
        {"generator",
            nlohmann::ordered_json{{"name", "par-beatdown"}, {"version", version()}, {"backend", tracker_backend},
                {"library", tracker_library}, {"library_version", openmpt::string::get("library_version")}}},
        {"source", make_source_json(source)}, {"render", make_render_json(settings)},
        {"module", make_module_json(module, clock)}, {"timeline", make_timeline_json(clock)},
        {"events", make_events_json(events)}, {"features", make_features_json(features)},
        {"diagnostics", make_diagnostics_json(source)}};

    return root.dump(2) + "\n";
}

} // namespace

struct TrackerModule::Impl
{
    explicit Impl(std::string file);

    std::string metadata_value(const std::vector<std::string> &keys, const std::string &key) const;
    std::string order_kind(int order) const;
    int pattern_row_count(int pattern) const;
    void load_source_info();
    void load_module_info();
    TrackerClockInfo make_clock_info(const TrackerTimelineSettings &settings) const;
    std::vector<TrackerTimelineEvent> make_pattern_events(const TrackerTimelineSettings &settings) const;
    std::vector<TrackerFeatureFrame> make_feature_frames(const TrackerTimelineSettings &settings);

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
    size_bytes_{tracker_file_size(file_)},
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

int TrackerModule::Impl::pattern_row_count(int pattern) const
{
    if (pattern < 0 || pattern >= module_info_.pattern_count)
    {
        return 0;
    }
    return module_info_.patterns[static_cast<std::size_t>(pattern)].row_count;
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
    if (module_info_.channel_count <= 0 || module_info_.order_count <= 0 || module_info_.pattern_count <= 0)
    {
        throw std::runtime_error{"unsupported empty tracker module: " + file_};
    }

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

TrackerClockInfo TrackerModule::Impl::make_clock_info(const TrackerTimelineSettings &settings) const
{
    TrackerClockInfo info;
    if (!is_valid_time(source_.duration_seconds))
    {
        return info;
    }

    const auto end_seconds = window_end_seconds(source_.duration_seconds, settings);
    info.timeline.duration_seconds = end_seconds - settings.start_seconds;
    info.timeline.first_frame = 0;
    info.timeline.last_frame =
        std::max(info.timeline.first_frame, frame_at_output(info.timeline.duration_seconds, settings));
    info.timeline.frames = info.timeline.last_frame - info.timeline.first_frame + 1;

    for (const auto &order : module_info_.orders)
    {
        if (order.kind != "pattern")
        {
            continue;
        }

        const auto order_time = module_.get_time_at_position(order.index, 0);
        if (!is_valid_time(order_time))
        {
            continue;
        }

        if (is_in_window(order_time, end_seconds, settings))
        {
            const auto output_time = output_seconds_at(order_time, settings);
            const auto order_frame = frame_at_output(output_time, settings);
            info.orders.push_back(
                TrackerOrderClockInfo{order.index, order.pattern, order.kind, output_time, order_frame});
            info.events.push_back(
                TrackerTimelineEvent{"order", output_time, order_frame, order.index, order.pattern, 0});
            info.events.push_back(
                TrackerTimelineEvent{"pattern", output_time, order_frame, order.index, order.pattern, 0});
        }

        const auto rows = pattern_row_count(order.pattern);
        for (int row = 0; row < rows; ++row)
        {
            const auto row_time = module_.get_time_at_position(order.index, row);
            if (!is_valid_time(row_time) || !is_in_window(row_time, end_seconds, settings))
            {
                continue;
            }
            const auto output_row_time = output_seconds_at(row_time, settings);
            info.events.push_back(TrackerTimelineEvent{
                "row", output_row_time, frame_at_output(output_row_time, settings), order.index, order.pattern, row});
        }
    }

    return info;
}

std::vector<TrackerTimelineEvent> TrackerModule::Impl::make_pattern_events(
    const TrackerTimelineSettings &settings) const
{
    std::vector<TrackerTimelineEvent> events;
    const auto end_seconds = window_end_seconds(source_.duration_seconds, settings);
    for (const auto &order : module_info_.orders)
    {
        if (order.kind != "pattern")
        {
            continue;
        }

        const auto rows = pattern_row_count(order.pattern);
        for (int row = 0; row < rows; ++row)
        {
            const auto row_time = module_.get_time_at_position(order.index, row);
            if (!is_valid_time(row_time) || !is_in_window(row_time, end_seconds, settings))
            {
                continue;
            }

            const auto output_time = output_seconds_at(row_time, settings);
            const auto frame = frame_at_output(output_time, settings);
            for (int channel = 0; channel < module_info_.channel_count; ++channel)
            {
                const auto note =
                    module_.get_pattern_row_channel_command(order.pattern, row, channel, openmpt::module::command_note);
                const auto instrument = module_.get_pattern_row_channel_command(
                    order.pattern, row, channel, openmpt::module::command_instrument);
                const auto volume = module_.get_pattern_row_channel_command(
                    order.pattern, row, channel, openmpt::module::command_volume);
                const auto volume_effect = module_.get_pattern_row_channel_command(
                    order.pattern, row, channel, openmpt::module::command_volumeffect);
                const auto effect = module_.get_pattern_row_channel_command(
                    order.pattern, row, channel, openmpt::module::command_effect);
                const auto parameter = module_.get_pattern_row_channel_command(
                    order.pattern, row, channel, openmpt::module::command_parameter);
                const auto text =
                    trim_pattern_text(module_.format_pattern_row_channel(order.pattern, row, channel, 0, false));

                const auto has_note = note != 0 || instrument != 0;
                const auto has_effect = volume != 0 || volume_effect != 0 || effect != 0 || parameter != 0;
                if (!has_note && !has_effect && !has_pattern_text(text))
                {
                    continue;
                }

                const auto note_text = trim_pattern_text(module_.format_pattern_row_channel_command(
                    order.pattern, row, channel, openmpt::module::command_note));
                const auto effect_text = trim_pattern_text(module_.format_pattern_row_channel_command(
                    order.pattern, row, channel, openmpt::module::command_effect));

                if (has_note)
                {
                    TrackerTimelineEvent event{"note", output_time, frame, order.index, order.pattern, row};
                    event.channel = channel;
                    if (has_pattern_text(note_text))
                    {
                        event.note = note_text;
                    }
                    if (instrument != 0)
                    {
                        event.instrument = static_cast<int>(instrument);
                    }
                    if (volume != 0)
                    {
                        event.volume = static_cast<int>(volume);
                    }
                    event.text = text;
                    events.push_back(event);
                }

                if (has_effect)
                {
                    TrackerTimelineEvent event{"effect", output_time, frame, order.index, order.pattern, row};
                    event.channel = channel;
                    if (volume != 0)
                    {
                        event.volume = static_cast<int>(volume);
                    }
                    if (has_pattern_text(effect_text))
                    {
                        event.effect = effect_text;
                    }
                    if (effect != 0 || parameter != 0)
                    {
                        event.parameter = static_cast<int>(parameter);
                    }
                    event.text = text;
                    events.push_back(event);
                }
            }
        }
    }
    return events;
}

std::vector<TrackerFeatureFrame> TrackerModule::Impl::make_feature_frames(const TrackerTimelineSettings &settings)
{
    std::vector<TrackerFeatureFrame> features;
    if (settings.sample_rate <= 0 || settings.feature_hop_seconds <= 0.0)
    {
        return features;
    }

    const auto end_seconds = window_end_seconds(source_.duration_seconds, settings);
    const auto duration_seconds = end_seconds - settings.start_seconds;
    if (duration_seconds <= 0.0)
    {
        return features;
    }

    module_.set_position_seconds(settings.start_seconds);
    const auto hop_samples = std::max<std::size_t>(1U,
        static_cast<std::size_t>(
            std::llround(settings.feature_hop_seconds * static_cast<double>(settings.sample_rate))));
    auto samples = std::vector<float>(hop_samples * 2U);
    std::uintmax_t rendered_samples = 0;
    const auto limited_samples = has_duration_limit(settings)
        ? static_cast<std::uintmax_t>(std::llround(duration_seconds * static_cast<double>(settings.sample_rate)))
        : 0U;

    while (true)
    {
        auto request_samples = hop_samples;
        if (has_duration_limit(settings))
        {
            if (rendered_samples >= limited_samples)
            {
                break;
            }
            request_samples =
                std::min<std::size_t>(hop_samples, static_cast<std::size_t>(limited_samples - rendered_samples));
        }

        const auto rendered = module_.read_interleaved_stereo(settings.sample_rate, request_samples, samples.data());
        if (rendered == 0U)
        {
            break;
        }

        double sum_squares = 0.0;
        double peak = 0.0;
        const auto sample_count = rendered * 2U;
        for (std::size_t index = 0; index < sample_count; ++index)
        {
            const auto sample = static_cast<double>(samples[index]);
            sum_squares += sample * sample;
            peak = std::max(peak, std::abs(sample));
        }

        TrackerFeatureFrame feature;
        feature.time_seconds = static_cast<double>(rendered_samples) / static_cast<double>(settings.sample_rate);
        feature.frame = frame_at_output(feature.time_seconds, settings);
        feature.rms = std::sqrt(sum_squares / static_cast<double>(sample_count));
        feature.peak = peak;
        feature.active_channels = module_.get_current_playing_channels();
        feature.channel_vu_mono.reserve(static_cast<std::size_t>(module_info_.channel_count));
        for (int channel = 0; channel < module_info_.channel_count; ++channel)
        {
            feature.channel_vu_mono.push_back(module_.get_current_channel_vu_mono(channel));
        }
        features.push_back(feature);
        rendered_samples += rendered;
    }

    return features;
}

TrackerModule::TrackerModule(std::string file)
try :
    impl_{std::make_unique<Impl>(file)}
{
}
catch (const openmpt::exception &exception)
{
    (void) exception;
    throw std::runtime_error{"could not load tracker file: " + file};
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

TrackerClockInfo TrackerModule::clock_info(const TrackerTimelineSettings &settings)
{
    return impl_->make_clock_info(settings);
}

std::vector<TrackerTimelineEvent> TrackerModule::pattern_events(const TrackerTimelineSettings &settings)
{
    return impl_->make_pattern_events(settings);
}

std::vector<TrackerFeatureFrame> TrackerModule::feature_frames(const TrackerTimelineSettings &settings)
{
    return impl_->make_feature_frames(settings);
}

std::string tracker_timeline_schema_shell(const TrackerTimelineSettings &settings)
{
    return dump_tracker_timeline(settings, nullptr, nullptr, nullptr, nullptr, nullptr);
}

std::string tracker_timeline_json(TrackerModule &module, const TrackerTimelineSettings &settings)
{
    const auto clock = settings.include_timeline ? module.clock_info(settings) : TrackerClockInfo{};
    auto events = settings.include_timeline ? clock.events : std::vector<TrackerTimelineEvent>{};
    if (settings.include_pattern_events)
    {
        auto pattern_events = module.pattern_events(settings);
        events.insert(events.end(), pattern_events.begin(), pattern_events.end());
    }
    const auto include_events = settings.include_timeline || settings.include_pattern_events;
    const auto features =
        settings.include_features ? module.feature_frames(settings) : std::vector<TrackerFeatureFrame>{};
    return dump_tracker_timeline(settings, &module.source(), settings.include_module ? &module.module_info() : nullptr,
        settings.include_timeline ? &clock : nullptr, include_events ? &events : nullptr,
        settings.include_features ? &features : nullptr);
}

std::string tracker_timeline_from_file(const std::string &file, const TrackerTimelineSettings &settings)
{
    TrackerModule module{file};
    return tracker_timeline_json(module, settings);
}

} // namespace par_beatdown
