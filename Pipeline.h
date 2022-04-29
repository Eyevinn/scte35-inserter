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
        const std::chrono::minutes spliceInterval,
        const std::chrono::minutes spliceDuration);

    ~Pipeline();

    void run();
    void stop();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
