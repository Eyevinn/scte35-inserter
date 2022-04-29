#include "Pipeline.h"
#include <chrono>
#include <csignal>
#include <cstdint>
#include <glib-2.0/glib.h>
#include <unistd.h>
#include <utility>

namespace
{

const char* usageString = "Usage: scte35-insert -i <MPEG-TS input address:port> -o <MPEG-TS output address:port> -n "
                          "<SCTE-35 splice interval> -d <SCTE-35 splice duration> -b [MPEG-TS buffer size ms]";

GMainLoop* mainLoop = nullptr;
std::unique_ptr<Pipeline> pipeline;

void intSignalHandler(int32_t)
{
    g_main_loop_quit(mainLoop);
}

std::pair<std::string, uint32_t> splitAddressPort(const char* address)
{
    const size_t copySize = strnlen(address, 1024) + 1;
    auto addressCopy = std::make_unique<char[]>(copySize);
    strncpy(addressCopy.get(), address, copySize);

    char* next = nullptr;
    auto result = strtok_r(addressCopy.get(), ":", &next);
    if (!result || !next)
    {
        return {};
    }

    return {result, std::strtoul(next, nullptr, 10)};
}

} // namespace

int32_t main(int32_t argc, char** argv)
{
    {
        struct sigaction sigactionData = {};
        sigactionData.sa_handler = intSignalHandler;
        sigactionData.sa_flags = 0;
        sigemptyset(&sigactionData.sa_mask);
        sigaction(SIGINT, &sigactionData, nullptr);
    }

    std::pair<std::string, uint32_t> inAddress;
    std::pair<std::string, uint32_t> outAddress;
    int32_t getOptResult;
    std::chrono::minutes spliceInterval(0);
    std::chrono::minutes spliceDuration(0);
    std::chrono::milliseconds mpegTsBufferSize(1000);

    while ((getOptResult = getopt(argc, argv, "i:o:n:d:b:")) != -1)
    {
        switch (getOptResult)
        {
        case 'i':
            inAddress = splitAddressPort(optarg);
            break;
        case 'o':
            outAddress = splitAddressPort(optarg);
            break;
        case 'n':
            spliceInterval = std::chrono::minutes(std::strtoull(optarg, nullptr, 10));
            break;
        case 'd':
            spliceDuration = std::chrono::minutes(std::strtoull(optarg, nullptr, 10));
            break;
        case 'b':
            mpegTsBufferSize = std::chrono::milliseconds(std::strtoull(optarg, nullptr, 10));
            break;
        default:
            printf("%s\n", usageString);
            return 1;
        }
    }

    if (inAddress.first.empty() || inAddress.second == 0 || outAddress.first.empty() || outAddress.second == 0 ||
        spliceInterval.count() == 0 || spliceDuration.count() == 0)
    {
        printf("%s\n", usageString);
        return 1;
    }

    mainLoop = g_main_loop_new(nullptr, FALSE);

    pipeline = std::make_unique<Pipeline>(inAddress, outAddress, mpegTsBufferSize, spliceInterval, spliceDuration);
    pipeline->run();

    g_main_loop_run(mainLoop);

    return 0;
}
