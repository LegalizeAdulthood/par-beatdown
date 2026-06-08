#include <ParBeatdown/Version.h>

#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
#include <ParBeatdown/TrackerTimeline.h>

#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#endif

#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
namespace
{

struct ToolOptions
{
    std::string input;
    std::string output;
    par_beatdown::TrackerTimelineSettings settings;
    bool include_filter_seen{false};
};

std::string require_value(int &index, int argc, char *argv[], const std::string &option)
{
    if (++index == argc)
    {
        throw std::runtime_error{"missing value after " + option};
    }
    return argv[index];
}

double parse_number(const std::string &option, const std::string &text)
{
    std::size_t parsed = 0;
    double value = 0.0;
    try
    {
        value = std::stod(text, &parsed);
    }
    catch (const std::exception &)
    {
        throw std::runtime_error{"invalid " + option + ": " + text};
    }
    if (parsed != text.size() || !std::isfinite(value))
    {
        throw std::runtime_error{"invalid " + option + ": " + text};
    }
    return value;
}

double parse_positive_number(const std::string &option, const std::string &text)
{
    const auto value = parse_number(option, text);
    if (value <= 0.0)
    {
        throw std::runtime_error{"invalid " + option + ": value must be positive"};
    }
    return value;
}

void reset_optional_sections(ToolOptions &options)
{
    if (!options.include_filter_seen)
    {
        options.settings.include_module = false;
        options.settings.include_timeline = false;
        options.settings.include_pattern_events = false;
        options.settings.include_features = false;
        options.include_filter_seen = true;
    }
}

void add_include(ToolOptions &options, const std::string &name)
{
    reset_optional_sections(options);

    if (name == "source")
    {
        return;
    }
    if (name == "module")
    {
        options.settings.include_module = true;
        return;
    }
    if (name == "timeline")
    {
        options.settings.include_timeline = true;
        return;
    }
    if (name == "events")
    {
        options.settings.include_pattern_events = true;
        return;
    }
    if (name == "features")
    {
        options.settings.include_features = true;
        return;
    }
    throw std::runtime_error{"unknown include section: " + name};
}

ToolOptions parse_options(int argc, char *argv[])
{
    ToolOptions options;
    if (argc < 2)
    {
        throw std::runtime_error{"missing input path"};
    }

    options.input = argv[1];
    for (int index = 2; index < argc; ++index)
    {
        const std::string argument{argv[index]};
        if (argument == "-o")
        {
            options.output = require_value(index, argc, argv, "-o");
        }
        else if (argument == "--include")
        {
            add_include(options, require_value(index, argc, argv, "--include"));
        }
        else if (argument == "--fps")
        {
            options.settings.fps = parse_positive_number("--fps", require_value(index, argc, argv, "--fps"));
        }
        else if (argument == "--offset")
        {
            options.settings.offset_seconds = parse_number("--offset", require_value(index, argc, argv, "--offset"));
        }
        else if (argument == "--feature-hop")
        {
            options.settings.feature_hop_seconds =
                parse_positive_number("--feature-hop", require_value(index, argc, argv, "--feature-hop"));
        }
        else
        {
            throw std::runtime_error{"unknown argument: " + argument};
        }
    }

    return options;
}

int write_timeline(const ToolOptions &options)
{
    if (options.input.empty())
    {
        throw std::runtime_error{"missing input path"};
    }
    if (options.output.empty())
    {
        throw std::runtime_error{"missing output path"};
    }

    std::ofstream output{options.output, std::ios::binary};
    if (!output)
    {
        throw std::runtime_error{"could not open output file: " + options.output};
    }

    output << par_beatdown::tracker_timeline_from_file(options.input, options.settings);
    return output ? 0 : 1;
}

} // namespace
#endif

int main(int argc, char *argv[])
{
#ifdef PAR_BEATDOWN_ENABLE_TRACKER_FILE
    try
    {
        return write_timeline(parse_options(argc, argv));
    }
    catch (const std::exception &exception)
    {
        std::cerr << exception.what() << "\n";
        return 1;
    }
#else
    (void) argc;
    (void) argv;
#endif

    return par_beatdown::version()[0] == '\0' ? 1 : 0;
}
