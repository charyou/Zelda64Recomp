#ifndef ZELDA_DEVELOPER_INPUT_H
#define ZELDA_DEVELOPER_INPUT_H

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "json/json.hpp"

namespace zelda64 {

// Opt-in controller-read playback for visual validation, not a save-state or
// deterministic simulation system. Once opted in, physical input stays ignored.
class DeveloperInputPlayback {
    struct Step {
        uint32_t reads;
        uint16_t buttons;
        float x;
        float y;
    };
    bool enabled = false;
    std::vector<Step> steps;
    size_t index = 0;
    uint32_t remaining = 0;
    uint64_t total_reads = 0;

    static double number(const nlohmann::json& step, const char* key, double minimum, double maximum, bool integer) {
        const auto& value = step.at(key);
        if (!value.is_number() || (integer && !value.is_number_integer())) {
            throw std::runtime_error(std::string(key) + " has the wrong numeric type");
        }
        const double result = value.get<double>();
        if (!std::isfinite(result) || result < minimum || result > maximum) {
            throw std::runtime_error(std::string(key) + " is out of range");
        }
        return result;
    }

public:
    explicit DeveloperInputPlayback(const char* path) {
        if (path == nullptr || path[0] == '\0') {
            return;
        }
        enabled = true;
        try {
            const std::string path_string(path);
            const std::filesystem::path input_path(std::u8string(path_string.begin(), path_string.end()));
            if (std::filesystem::file_size(input_path) > 1024 * 1024) {
                throw std::runtime_error("sequence file exceeds 1 MiB");
            }
            std::ifstream input(input_path);
            if (!input) {
                throw std::runtime_error("cannot open sequence file");
            }
            const auto document = nlohmann::json::parse(input);
            if (!document.is_array() || document.empty() || document.size() > 4096) {
                throw std::runtime_error("expected an array of 1 to 4096 steps");
            }
            std::vector<Step> parsed;
            uint64_t length = 0;
            for (const auto& item : document) {
                if (!item.is_object() || item.size() != 4) {
                    throw std::runtime_error("each step must contain reads, buttons, x and y only");
                }
                Step step{
                    static_cast<uint32_t>(number(item, "reads", 1, 1000000, true)),
                    static_cast<uint16_t>(number(item, "buttons", 0, 65535, true)),
                    static_cast<float>(number(item, "x", -1, 1, false)),
                    static_cast<float>(number(item, "y", -1, 1, false)),
                };
                length += step.reads;
                if (length > 10000000) {
                    throw std::runtime_error("sequence exceeds 10000000 controller reads");
                }
                parsed.push_back(step);
            }
            steps = std::move(parsed);
            remaining = steps.front().reads;
            fprintf(stderr, "[Dev input] Started %zu steps, %llu controller-0 reads: %s\n",
                steps.size(), static_cast<unsigned long long>(length), path);
        }
        catch (const std::exception& error) {
            // A malformed test must not silently become an uncontrolled run.
            steps.clear();
            fprintf(stderr, "[Dev input] Rejected %s: %s. Controller 0 remains neutral.\n", path, error.what());
        }
        fflush(stderr);
    }

    bool read(uint16_t* buttons, float* x, float* y) {
        if (!enabled) {
            return false;
        }
        *buttons = 0;
        *x = 0.0f;
        *y = 0.0f;
        if (index < steps.size()) {
            const Step& step = steps[index];
            *buttons = step.buttons;
            *x = step.x;
            *y = step.y;
            total_reads++;
            if (--remaining == 0) {
                index++;
                if (index < steps.size()) {
                    remaining = steps[index].reads;
                }
                else {
                    fprintf(stderr, "[Dev input] Finished after %llu controller-0 reads; subsequent input is neutral.\n",
                        static_cast<unsigned long long>(total_reads));
                    fflush(stderr);
                }
            }
        }
        return true;
    }
};

} // namespace zelda64

#endif
