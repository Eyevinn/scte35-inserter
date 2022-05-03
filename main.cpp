#include "Pipeline.h"
#include <array>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <getopt.h>
#include <glib-2.0/glib.h>
#include <unistd.h>
#include <utility>

namespace
{

const char* usageString = "Usage: scte35-inserter -i <MPEG-TS input address:port> -o [MPEG-TS output address:port] -n "
                          "<SCTE-35 splice interval s> -d <SCTE-35 splice duration s> [--immediate] --file [output "
                          "file name (instead of UDP output)]";

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
    std::chrono::seconds spliceInterval(0);
    std::chrono::seconds spliceDuration(0);
    std::string outputFileName;

    int32_t immediate = 0;

    std::array<option, 7> longOptions;
    longOptions[0] = {"file", required_argument, 0, 'f'};
    longOptions[1] = {"immediate", no_argument, &immediate, 1};
    longOptions[2] = {"input", required_argument, 0, 'i'};
    longOptions[3] = {"output", required_argument, 0, 'o'};
    longOptions[4] = {"interval", required_argument, 0, 'n'};
    longOptions[5] = {"duration", required_argument, 0, 'd'};
    longOptions[6] = {0, 0, 0, 0};

    int32_t optionIndex = 0;

    while ((getOptResult = getopt_long(argc, argv, "f:i:o:n:d:", longOptions.data(), &optionIndex)) != -1)
    {
        switch (getOptResult)
        {
        case 0:
            break;
        case 'i':
            inAddress = splitAddressPort(optarg);
            break;
        case 'o':
            outAddress = splitAddressPort(optarg);
            break;
        case 'n':
            spliceInterval = std::chrono::seconds(std::strtoull(optarg, nullptr, 10));
            break;
        case 'd':
            spliceDuration = std::chrono::seconds(std::strtoull(optarg, nullptr, 10));
            break;
        case 'f':
            outputFileName = optarg;
            break;
        default:
            printf("%s\n", usageString);
            return 1;
        }
    }

    if (inAddress.first.empty() || inAddress.second == 0 ||
        ((outAddress.first.empty() || outAddress.second == 0) && outputFileName.empty()) ||
        ((!outAddress.first.empty() && outAddress.second != 0) && !outputFileName.empty()) ||
        spliceInterval.count() == 0 || spliceDuration.count() == 0)
    {
        printf("%s\n", usageString);
        return 1;
    }

    mainLoop = g_main_loop_new(nullptr, FALSE);

    std::chrono::milliseconds mpegTsBufferSize(1000);
    pipeline = std::make_unique<Pipeline>(inAddress,
        outAddress,
        mpegTsBufferSize,
        spliceInterval,
        spliceDuration,
        immediate == 1,
        outputFileName);
    pipeline->run();

    g_main_loop_run(mainLoop);

    return 0;
}
