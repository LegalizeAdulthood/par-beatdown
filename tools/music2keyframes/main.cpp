#include <ParBeatdown/Version.h>

#include <nlohmann/json.hpp>

#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{

using Json = nlohmann::ordered_json;

constexpr const char *CONFIG_SCHEMA = "par-beatdown.music2keyframes";
constexpr const char *OVERLAY_SCHEMA = "par-beatdown.music2keyframes-overlay";

struct ToolOptions
{
    std::string base_animation;
    std::string timeline;
    std::string adapter_config;
    std::string output;
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

Json make_empty_overlay(const ToolOptions &options)
{
    Json overlay = Json::object();
    overlay["schema"] = OVERLAY_SCHEMA;
    overlay["version"] = 1;
    overlay["generator"] = Json::object();
    overlay["generator"]["name"] = "music2keyframes";
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

int write_overlay(const ToolOptions &options)
{
    (void) read_json_file(options.base_animation, "base animation");
    (void) read_json_file(options.timeline, "timeline");
    const auto config = read_json_file(options.adapter_config, "adapter config");
    validate_adapter_config(config);

    std::ofstream output{options.output, std::ios::binary};
    if (!output)
    {
        throw std::runtime_error{"could not open output file: " + options.output};
    }

    output << make_empty_overlay(options).dump(2) << "\n";
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
