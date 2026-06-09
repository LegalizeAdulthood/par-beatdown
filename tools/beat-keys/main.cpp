#include <ParBeatdown/Version.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

using Json = nlohmann::ordered_json;

constexpr const char *CONFIG_SCHEMA = "par-beatdown.beat-keys";
constexpr const char *OVERLAY_SCHEMA = "par-beatdown.beat-keys-overlay";

struct ToolOptions
{
    std::string base_animation;
    std::string timeline;
    std::string adapter_config;
    std::string output;
};

enum class BindingSource
{
    Rms,
    Peak,
    NotePulse,
    EffectPulse,
    RowPulse
};

struct Binding
{
    BindingSource binding_source{BindingSource::Rms};
    std::string source;
    std::string target;
    std::string op;
    double scale{1.0};
    double offset{0.0};
    double decay_seconds{0.0};
    bool has_clamp{false};
    double clamp_min{0.0};
    double clamp_max{0.0};
};

struct AdapterSettings
{
    double fps{30.0};
};

struct Keyframe
{
    int frame{0};
    int order{0};
    Json json;
};

std::string require_value(int &index, int argc, char *argv[], const std::string &option)
{
    if (++index == argc)
    {
        throw std::runtime_error{"missing value after " + option};
    }
    return argv[index];
}

ToolOptions parse_options(int argc, char *argv[])
{
    ToolOptions options;
    if (argc < 2)
    {
        throw std::runtime_error{"missing base animation path"};
    }
    if (argc < 3)
    {
        throw std::runtime_error{"missing timeline path"};
    }
    if (argc < 4)
    {
        throw std::runtime_error{"missing adapter config path"};
    }

    options.base_animation = argv[1];
    options.timeline = argv[2];
    options.adapter_config = argv[3];
    for (int index = 4; index < argc; ++index)
    {
        const std::string argument{argv[index]};
        if (argument == "-o")
        {
            options.output = require_value(index, argc, argv, "-o");
        }
        else
        {
            throw std::runtime_error{"unknown argument: " + argument};
        }
    }
    if (options.output.empty())
    {
        throw std::runtime_error{"missing output path"};
    }

    return options;
}

Json read_json_file(const std::string &path, const std::string &label)
{
    std::ifstream input{path, std::ios::binary};
    if (!input)
    {
        throw std::runtime_error{"could not open " + label + ": " + path};
    }

    try
    {
        Json json;
        input >> json;
        return json;
    }
    catch (const std::exception &)
    {
        throw std::runtime_error{"could not parse " + label + ": " + path};
    }
}

std::string config_schema(const Json &config)
{
    const auto schema = config.find("schema");
    if (schema == config.end() || !schema->is_string())
    {
        return {};
    }
    return schema->get<std::string>();
}

int config_version(const Json &config)
{
    const auto version = config.find("version");
    if (version == config.end() || !version->is_number_integer())
    {
        return 0;
    }
    return version->get<int>();
}

void validate_adapter_config(const Json &config)
{
    if (!config.is_object())
    {
        throw std::runtime_error{"adapter config must be an object"};
    }

    const auto schema = config_schema(config);
    if (schema != CONFIG_SCHEMA)
    {
        throw std::runtime_error{"unsupported adapter config schema: " + schema};
    }

    const auto version = config_version(config);
    if (version != 1)
    {
        throw std::runtime_error{"unsupported adapter config version: " + std::to_string(version)};
    }
}

const Json &require_array_field(const Json &json, const std::string &field, const std::string &label)
{
    const auto value = json.find(field);
    if (value == json.end() || !value->is_array())
    {
        throw std::runtime_error{"missing " + label + "." + field};
    }
    return *value;
}

const Json &require_object_field(const Json &json, const std::string &field, const std::string &label)
{
    const auto value = json.find(field);
    if (value == json.end() || !value->is_object())
    {
        throw std::runtime_error{"missing " + label + "." + field};
    }
    return *value;
}

std::string require_string_field(const Json &json, const std::string &field, const std::string &label)
{
    const auto value = json.find(field);
    if (value == json.end() || !value->is_string())
    {
        throw std::runtime_error{"missing " + label + "." + field};
    }
    return value->get<std::string>();
}

int require_integer_field(const Json &json, const std::string &field, const std::string &label)
{
    const auto value = json.find(field);
    if (value == json.end() || !value->is_number_integer())
    {
        throw std::runtime_error{"missing " + label + "." + field};
    }
    return value->get<int>();
}

double require_number_field(const Json &json, const std::string &field, const std::string &label)
{
    const auto value = json.find(field);
    if (value == json.end() || !value->is_number())
    {
        throw std::runtime_error{"missing " + label + "." + field};
    }
    return value->get<double>();
}

double optional_number_field(const Json &json, const std::string &field, double default_value)
{
    const auto value = json.find(field);
    if (value == json.end())
    {
        return default_value;
    }
    if (!value->is_number())
    {
        throw std::runtime_error{"invalid binding." + field};
    }
    return value->get<double>();
}

bool string_ends_with(const std::string &text, const std::string &suffix)
{
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

BindingSource parse_binding_source(const std::string &source)
{
    if (source == "music.rms")
    {
        return BindingSource::Rms;
    }
    if (source == "music.peak")
    {
        return BindingSource::Peak;
    }
    if (source == "music.note_pulse")
    {
        return BindingSource::NotePulse;
    }
    if (source == "music.effect_pulse")
    {
        return BindingSource::EffectPulse;
    }
    if (source == "music.row_pulse")
    {
        return BindingSource::RowPulse;
    }
    if (string_ends_with(source, "_pulse"))
    {
        throw std::runtime_error{"unsupported event source: " + source};
    }
    throw std::runtime_error{"unsupported continuous source: " + source};
}

bool is_continuous_source(BindingSource source)
{
    return source == BindingSource::Rms || source == BindingSource::Peak;
}

bool is_event_source(BindingSource source)
{
    return !is_continuous_source(source);
}

void validate_op(const std::string &op)
{
    if (op != "add" && op != "multiply" && op != "replace")
    {
        throw std::runtime_error{"unsupported binding op: " + op};
    }
}

Binding parse_binding(const Json &json, int index)
{
    if (!json.is_object())
    {
        throw std::runtime_error{"binding must be an object: " + std::to_string(index)};
    }

    Binding binding;
    const std::string label = "binding " + std::to_string(index);
    binding.source = require_string_field(json, "source", label);
    binding.binding_source = parse_binding_source(binding.source);
    binding.target = require_string_field(json, "target", label);
    binding.op = require_string_field(json, "op", label);
    validate_op(binding.op);
    binding.scale = optional_number_field(json, "scale", 1.0);
    binding.offset = optional_number_field(json, "offset", 0.0);
    binding.decay_seconds = optional_number_field(json, "decay_seconds", 0.0);
    if (binding.decay_seconds < 0.0)
    {
        throw std::runtime_error{"invalid binding.decay_seconds"};
    }

    const auto clamp = json.find("clamp");
    if (clamp != json.end())
    {
        if (!clamp->is_object())
        {
            throw std::runtime_error{"invalid binding.clamp"};
        }
        binding.has_clamp = true;
        binding.clamp_min = require_number_field(*clamp, "min", label + ".clamp");
        binding.clamp_max = require_number_field(*clamp, "max", label + ".clamp");
        if (binding.clamp_min > binding.clamp_max)
        {
            throw std::runtime_error{"invalid binding.clamp range"};
        }
    }

    return binding;
}

std::vector<Binding> parse_bindings(const Json &config)
{
    std::vector<Binding> bindings;
    const auto &json_bindings = require_array_field(config, "bindings", "adapter config");
    for (std::size_t index = 0; index < json_bindings.size(); ++index)
    {
        bindings.push_back(parse_binding(json_bindings[index], static_cast<int>(index)));
    }
    return bindings;
}

AdapterSettings parse_adapter_settings(const Json &config)
{
    AdapterSettings settings;
    const auto &source = require_object_field(config, "source", "adapter config");
    settings.fps = require_number_field(source, "fps", "adapter config.source");
    if (settings.fps <= 0.0)
    {
        throw std::runtime_error{"invalid adapter config.source.fps"};
    }
    return settings;
}

const char *feature_field(BindingSource source)
{
    switch (source)
    {
    case BindingSource::Rms:
        return "rms";
    case BindingSource::Peak:
        return "peak";
    case BindingSource::NotePulse:
    case BindingSource::EffectPulse:
    case BindingSource::RowPulse:
        break;
    }
    return "";
}

const char *event_kind(BindingSource source)
{
    switch (source)
    {
    case BindingSource::NotePulse:
        return "note";
    case BindingSource::EffectPulse:
        return "effect";
    case BindingSource::RowPulse:
        return "row";
    case BindingSource::Rms:
    case BindingSource::Peak:
        break;
    }
    return "";
}

double round_value(double value)
{
    return std::round(value * 1000000.0) / 1000000.0;
}

double apply_binding(double input, const Binding &binding)
{
    auto value = input * binding.scale + binding.offset;
    if (binding.has_clamp)
    {
        value = std::clamp(value, binding.clamp_min, binding.clamp_max);
    }
    return round_value(value);
}

Json make_keyframe(int frame, double input, const Binding &binding)
{
    Json keyframe = Json::object();
    keyframe["frame"] = frame;
    keyframe["target"] = binding.target;
    keyframe["op"] = binding.op;
    keyframe["value"] = apply_binding(input, binding);
    keyframe["source"] = binding.source;
    return keyframe;
}

Json make_zero_keyframe(int frame, const Binding &binding)
{
    Json keyframe = Json::object();
    keyframe["frame"] = frame;
    keyframe["target"] = binding.target;
    keyframe["op"] = binding.op;
    keyframe["value"] = 0.0;
    keyframe["source"] = binding.source;
    return keyframe;
}

void append_keyframe(std::vector<Keyframe> &keyframes, int &order, Json keyframe)
{
    const auto frame = keyframe["frame"].get<int>();
    keyframes.push_back(Keyframe{frame, order++, std::move(keyframe)});
}

void append_continuous_keyframes(
    std::vector<Keyframe> &keyframes, int &order, const Json &timeline, const std::vector<Binding> &bindings)
{
    const auto &features = require_array_field(timeline, "features", "timeline");
    for (const auto &feature : features)
    {
        if (!feature.is_object())
        {
            throw std::runtime_error{"timeline feature must be an object"};
        }
        const auto frame = require_integer_field(feature, "frame", "timeline feature");
        for (const auto &binding : bindings)
        {
            if (!is_continuous_source(binding.binding_source))
            {
                continue;
            }
            const auto *field = feature_field(binding.binding_source);
            const auto value = require_number_field(feature, field, "timeline feature");
            append_keyframe(keyframes, order, make_keyframe(frame, value, binding));
        }
    }
}

int decay_frame_count(const Binding &binding, double fps)
{
    if (binding.decay_seconds <= 0.0)
    {
        return 1;
    }
    return std::max(1, static_cast<int>(std::ceil(binding.decay_seconds * fps)));
}

double decay_value(int step, const Binding &binding, double fps, double count)
{
    if (step == 0)
    {
        return count;
    }
    if (binding.decay_seconds <= 0.0)
    {
        return 0.0;
    }
    const auto elapsed_seconds = static_cast<double>(step) / fps;
    return count * std::exp(-elapsed_seconds / binding.decay_seconds);
}

std::map<int, int> count_events_by_frame(const Json &timeline, const Binding &binding)
{
    std::map<int, int> counts;
    const auto &events = require_array_field(timeline, "events", "timeline");
    for (const auto &event : events)
    {
        if (!event.is_object())
        {
            throw std::runtime_error{"timeline event must be an object"};
        }
        const auto kind = require_string_field(event, "kind", "timeline event");
        if (kind == event_kind(binding.binding_source))
        {
            const auto frame = require_integer_field(event, "frame", "timeline event");
            ++counts[frame];
        }
    }
    return counts;
}

void append_event_keyframes(std::vector<Keyframe> &keyframes, int &order, const Json &timeline,
    const std::vector<Binding> &bindings, const AdapterSettings &settings)
{
    for (const auto &binding : bindings)
    {
        if (!is_event_source(binding.binding_source))
        {
            continue;
        }

        std::map<int, double> frame_values;
        std::set<int> return_frames;
        const auto decay_frames = decay_frame_count(binding, settings.fps);
        for (const auto &[frame, count] : count_events_by_frame(timeline, binding))
        {
            for (int step = 0; step < decay_frames; ++step)
            {
                frame_values[frame + step] += decay_value(step, binding, settings.fps, static_cast<double>(count));
            }
            return_frames.insert(frame + decay_frames);
        }

        for (const auto &[frame, value] : frame_values)
        {
            append_keyframe(keyframes, order, make_keyframe(frame, value, binding));
        }
        for (const auto frame : return_frames)
        {
            if (frame_values.find(frame) == frame_values.end())
            {
                append_keyframe(keyframes, order, make_zero_keyframe(frame, binding));
            }
        }
    }
}

Json make_keyframes(const Json &timeline, const std::vector<Binding> &bindings, const AdapterSettings &settings)
{
    std::vector<Keyframe> keyframes;
    auto order = 0;
    append_continuous_keyframes(keyframes, order, timeline, bindings);
    append_event_keyframes(keyframes, order, timeline, bindings, settings);
    std::stable_sort(keyframes.begin(), keyframes.end(), [](const Keyframe &left, const Keyframe &right)
        { return left.frame == right.frame ? left.order < right.order : left.frame < right.frame; });

    Json json_keyframes = Json::array();
    for (const auto &keyframe : keyframes)
    {
        json_keyframes.push_back(keyframe.json);
    }
    return json_keyframes;
}

Json make_empty_overlay(const ToolOptions &options)
{
    Json overlay = Json::object();
    overlay["schema"] = OVERLAY_SCHEMA;
    overlay["version"] = 1;
    overlay["generator"] = Json::object();
    overlay["generator"]["name"] = "beat-keys";
    overlay["generator"]["version"] = par_beatdown::version();
    overlay["source"] = Json::object();
    overlay["source"]["base_animation"] = options.base_animation;
    overlay["source"]["timeline"] = options.timeline;
    overlay["source"]["adapter_config"] = options.adapter_config;
    overlay["keyframes"] = Json::array();
    overlay["diagnostics"] = Json::object();
    overlay["diagnostics"]["warnings"] = Json::array();
    return overlay;
}

Json make_overlay(const ToolOptions &options, const Json &timeline, const Json &config)
{
    auto overlay = make_empty_overlay(options);
    overlay["keyframes"] = make_keyframes(timeline, parse_bindings(config), parse_adapter_settings(config));
    return overlay;
}

int write_overlay(const ToolOptions &options)
{
    (void) read_json_file(options.base_animation, "base animation");
    const auto timeline = read_json_file(options.timeline, "timeline");
    const auto config = read_json_file(options.adapter_config, "adapter config");
    validate_adapter_config(config);

    std::ofstream output{options.output, std::ios::binary};
    if (!output)
    {
        throw std::runtime_error{"could not open output file: " + options.output};
    }

    output << make_overlay(options, timeline, config).dump(2) << "\n";
    return output ? 0 : 1;
}

} // namespace

int main(int argc, char *argv[])
{
    try
    {
        return write_overlay(parse_options(argc, argv));
    }
    catch (const std::exception &exception)
    {
        std::cerr << exception.what() << "\n";
        return 1;
    }
}
