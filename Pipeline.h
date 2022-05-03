#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>

class Pipeline
{
public:
    explicit Pipeline(const std::pair<std::string, uint32_t>& inputAddress,
        const std::pair<std::string, uint32_t>& outputAddress,
        const std::chrono::milliseconds mpegTsBufferSize,
        const std::chrono::seconds spliceInterval,
        const std::chrono::seconds spliceDuration,
        const bool immediate,
        const std::string& outputFile);

    ~Pipeline();

    void run();
    void stop();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
