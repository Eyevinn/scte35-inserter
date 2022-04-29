#define GST_USE_UNSTABLE_API 1

#include "Pipeline.h"
#include "Logger.h"
#include "utils/ScopedGLibObject.h"
#include "utils/ScopedGstObject.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <gst/gst.h>
#include <gst/mpegts/mpegts.h>

class Pipeline::Impl
{
public:
    Impl(const std::pair<std::string, uint32_t>& inputAddress,
        const std::pair<std::string, uint32_t>& outputAddress,
        const std::chrono::milliseconds mpegTsBufferSize,
        const std::chrono::minutes spliceInterval,
        const std::chrono::minutes spliceDuration);
    ~Impl();

    void run();
    void stop();

    void onPipelineMessage(GstMessage* message);
    void onDemuxPadAdded(GstPad* newPad);

    static gboolean pipelineBusWatch(GstBus* /*bus*/, GstMessage* message, gpointer userData);
    static void demuxPadAddedCallback(GstElement* /*src*/, GstPad* newPad, gpointer userData);
    static gboolean sendScte35SpliceInCallback(gpointer userData);
    static gboolean sendScte35SpliceOutCallback(gpointer userData);

private:
    enum class ElementLabel
    {
        UDP_SOURCE,
        UDP_QUEUE,
        TS_PARSE,
        TS_DEMUX,
        H264_PARSE,
        VIDEO_PARSE_QUEUE,
        AAC_PARSE,
        AUDIO_PARSE_QUEUE,
        TS_MUX,
        TS_MUX_QUEUE,
        UDP_SINK
    };

    enum class SpliceType
    {
        IN,
        OUT
    };

    GstBus* pipelineMessageBus_;
    GstElement* pipeline_;
    std::map<ElementLabel, GstElement*> elements_;
    std::chrono::minutes spliceInterval_;
    std::chrono::minutes spliceDuration_;

    void makeElement(const ElementLabel elementLabel, const char* name, const char* element);
    void sendScte35Splice(const SpliceType spliceType);
};

Pipeline::Impl::Impl(const std::pair<std::string, uint32_t>& inputAddress,
    const std::pair<std::string, uint32_t>& outputAddress,
    const std::chrono::milliseconds mpegTsBufferSize,
    const std::chrono::minutes spliceInterval,
    const std::chrono::minutes spliceDuration)
    : pipelineMessageBus_(nullptr),
      spliceInterval_(spliceInterval),
      spliceDuration_(spliceDuration)
{
    gst_init(nullptr, nullptr);

    pipeline_ = gst_pipeline_new("scte35-insert-pipeline");
    makeElement(ElementLabel::UDP_SOURCE, "UDP_SOURCE", "udpsrc");
    makeElement(ElementLabel::UDP_QUEUE, "UDP_QUEUE", "queue");
    makeElement(ElementLabel::TS_PARSE, "TS_PARSE", "tsparse");
    makeElement(ElementLabel::TS_DEMUX, "TS_DEMUX", "tsdemux");
    makeElement(ElementLabel::H264_PARSE, "H264_PARSE", "h264parse");
    makeElement(ElementLabel::VIDEO_PARSE_QUEUE, "VIDEO_PARSE_QUEUE", "queue");
    makeElement(ElementLabel::AAC_PARSE, "AAC_PARSE", "aacparse");
    makeElement(ElementLabel::AUDIO_PARSE_QUEUE, "AUDIO_PARSE_QUEUE", "queue");
    makeElement(ElementLabel::TS_MUX, "TS_MUX", "mpegtsmux");
    makeElement(ElementLabel::TS_MUX_QUEUE, "TS_MUX_QUEUE", "queue");
    makeElement(ElementLabel::UDP_SINK, "UDP_SINK", "udpsink");

    for (const auto& entry : elements_)
    {
        if (!gst_bin_add(GST_BIN(pipeline_), entry.second))
        {
            Logger::log("Unable to add gst element");
            return;
        }
    }

    if (!gst_element_link_many(elements_[ElementLabel::UDP_SOURCE],
            elements_[ElementLabel::UDP_QUEUE],
            elements_[ElementLabel::TS_PARSE],
            elements_[ElementLabel::TS_DEMUX],
            nullptr))
    {
        g_printerr("Elements could not be linked.\n");
        return;
    }

    if (!gst_element_link_many(elements_[ElementLabel::TS_MUX],
            elements_[ElementLabel::TS_MUX_QUEUE],
            elements_[ElementLabel::UDP_SINK],
            nullptr))
    {
        Logger::log("Elements could not be linked.");
        return;
    }

    pipelineMessageBus_ = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
    gst_bus_add_watch(pipelineMessageBus_, reinterpret_cast<GstBusFunc>(pipelineBusWatch), this);

    g_signal_connect(elements_[ElementLabel::TS_DEMUX], "pad-added", G_CALLBACK(demuxPadAddedCallback), this);

    g_object_set(elements_[ElementLabel::UDP_SOURCE],
        "address",
        inputAddress.first.c_str(),
        "port",
        inputAddress.second,
        "auto-multicast",
        true,
        "buffer-size",
        825984,
        nullptr);

    g_object_set(elements_[ElementLabel::UDP_QUEUE],
        "min-threshold-time",
        std::chrono::nanoseconds(mpegTsBufferSize).count(),
        nullptr);

    g_object_set(elements_[ElementLabel::TS_MUX_QUEUE],
        "min-threshold-time",
        std::chrono::nanoseconds(mpegTsBufferSize).count(),
        nullptr);

    g_object_set(elements_[ElementLabel::UDP_SINK],
        "host",
        outputAddress.first.c_str(),
        "port",
        outputAddress.second,
        nullptr);
}

Pipeline::Impl::~Impl()
{
    gst_element_set_state(pipeline_, GST_STATE_NULL);

    if (pipelineMessageBus_)
    {
        gst_object_unref(pipelineMessageBus_);
    }

    gst_bus_remove_watch(pipelineMessageBus_);

    if (pipeline_)
    {
        gst_object_unref(pipeline_);
    }

    gst_deinit();
}

void Pipeline::Impl::onDemuxPadAdded(GstPad* newPad)
{
    utils::ScopedGstObject newPadCaps(gst_pad_get_current_caps(newPad));
    auto newPadStruct = gst_caps_get_structure(newPadCaps.get(), 0);
    auto newPadType = gst_structure_get_name(newPadStruct);

    Logger::log("Dynamic pad created, type %s", newPadType);

    if (g_str_has_prefix(newPadType, "video/x-h264"))
    {
        utils::ScopedGLibObject parseSinkPad(gst_element_get_static_pad(elements_[ElementLabel::H264_PARSE], "sink"));

        if (gst_pad_link(newPad, parseSinkPad.get()) != GST_PAD_LINK_OK)
        {
            Logger::log("pad link 1");
        }

        gst_element_link(elements_[ElementLabel::H264_PARSE], elements_[ElementLabel::VIDEO_PARSE_QUEUE]);

        utils::ScopedGLibObject parseQueueSourcePad(
            gst_element_get_static_pad(elements_[ElementLabel::VIDEO_PARSE_QUEUE], "src"));

        utils::ScopedGLibObject tsMuxSinkPad(
            gst_element_get_compatible_pad(elements_[ElementLabel::TS_MUX], parseQueueSourcePad.get(), nullptr));

        if (gst_pad_link(parseQueueSourcePad.get(), tsMuxSinkPad.get()) != GST_PAD_LINK_OK)
        {
            Logger::log("pad link 2");
        }
    }
    else if (g_str_has_prefix(newPadType, "video/mpeg"))
    {
    }
    else if (g_str_has_prefix(newPadType, "audio/mpeg"))
    {
        utils::ScopedGLibObject parseSinkPad(gst_element_get_static_pad(elements_[ElementLabel::AAC_PARSE], "sink"));

        if (gst_pad_link(newPad, parseSinkPad.get()) != GST_PAD_LINK_OK)
        {
            Logger::log("pad link 1");
        }

        gst_element_link(elements_[ElementLabel::AAC_PARSE], elements_[ElementLabel::AUDIO_PARSE_QUEUE]);

        utils::ScopedGLibObject parseQueueSourcePad(
            gst_element_get_static_pad(elements_[ElementLabel::AUDIO_PARSE_QUEUE], "src"));

        utils::ScopedGLibObject tsMuxSinkPad(
            gst_element_get_compatible_pad(elements_[ElementLabel::TS_MUX], parseQueueSourcePad.get(), nullptr));

        if (gst_pad_link(parseQueueSourcePad.get(), tsMuxSinkPad.get()) != GST_PAD_LINK_OK)
        {
            Logger::log("pad link 2");
        }
    }
    else if (g_str_has_prefix(newPadType, "audio/x-raw"))
    {
    }
    else
    {
        Logger::log("Unsupported MPEG-TS demux pad type %s", newPadType);
    }
}

void Pipeline::Impl::onPipelineMessage(GstMessage* message)
{
    switch (GST_MESSAGE_TYPE(message))
    {
    case GST_MESSAGE_STATE_CHANGED:
        if (GST_ELEMENT(message->src) == pipeline_)
        {
            GstState oldState;
            GstState newState;
            GstState pendingState;

            gst_message_parse_state_changed(message, &oldState, &newState, &pendingState);

            {
                auto dumpName = g_strconcat("state_changed-",
                    gst_element_state_get_name(oldState),
                    "_",
                    gst_element_state_get_name(newState),
                    nullptr);
                Logger::log("GST_MESSAGE_STATE_CHANGED %s", dumpName);
                g_free(dumpName);
            }

            if (newState == GST_STATE_PLAYING)
            {
                int64_t position = -1;
                if (!gst_element_query_position(elements_[ElementLabel::TS_PARSE], GST_FORMAT_TIME, &position))
                {
                    g_printerr("Could not query current position.\n");
                }
                Logger::log("Play started at position %" GST_TIME_FORMAT, GST_TIME_ARGS(position));
                g_timeout_add_seconds(std::chrono::seconds(spliceInterval_).count(), sendScte35SpliceOutCallback, this);
            }
        }
        break;

    case GST_MESSAGE_ERROR:
    {
        GError* err = nullptr;
        gchar* dbgInfo = nullptr;

        gst_message_parse_error(message, &err, &dbgInfo);
        Logger::log("ERROR from element %s: %s", GST_OBJECT_NAME(message->src), err->message);
        Logger::log("Debugging info: %s", dbgInfo ? dbgInfo : "none");
        g_error_free(err);
        g_free(dbgInfo);
    }
    break;

    case GST_MESSAGE_EOS:
        Logger::log("EOS received");
        break;

    case GST_MESSAGE_NEW_CLOCK:
        Logger::log("New pipeline clock");
        break;

    case GST_MESSAGE_CLOCK_LOST:
        Logger::log("Clock lost, restarting pipeline");
        if (gst_element_set_state(pipeline_, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE ||
            gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
        {
            Logger::log("Unable to restart the pipeline.");
            return;
        }
        break;

    default:
        break;
    }
}

void Pipeline::Impl::makeElement(const ElementLabel elementLabel, const char* name, const char* element)
{
    const auto& result = elements_.emplace(elementLabel, gst_element_factory_make(element, name));
    if (!result.first->second)
    {
        Logger::log("Unable to make gst element %s", element);
        return;
    }

    if (strncmp(element, "queue", 5) == 0)
    {
        g_object_set(result.first->second, "max-size-buffers", 0, nullptr);
        g_object_set(result.first->second, "max-size-bytes", 0, nullptr);
        g_object_set(result.first->second, "max-size-time", 0, nullptr);
    }
}

void Pipeline::Impl::sendScte35Splice(const Pipeline::Impl::SpliceType spliceType)
{
    int64_t position = -1;
    if (!gst_element_query_position(elements_[ElementLabel::TS_PARSE], GST_FORMAT_TIME, &position))
    {
        g_printerr("Could not query current position.\n");
    }
    Logger::log("sendScte35Splice: %s %" GST_TIME_FORMAT,
        spliceType == SpliceType::IN ? "IN" : "OUT",
        GST_TIME_ARGS(position));

    auto scteSit = spliceType == SpliceType::IN
        ? gst_mpegts_scte_splice_in_new(2, std::chrono::seconds(spliceDuration_ + spliceInterval_).count() * 90000)
        : gst_mpegts_scte_splice_out_new(1,
              GST_TIME_AS_SECONDS(position) * 90000,
              std::chrono::seconds(spliceDuration_).count() * 90000);

    utils::ScopedGstObject mpegTsSection(gst_mpegts_section_from_scte_sit(scteSit, 123));
    gst_mpegts_section_send_event(mpegTsSection.get(), elements_[ElementLabel::TS_MUX]);

    if (spliceType == SpliceType::IN)
    {
        g_timeout_add_seconds(std::chrono::seconds(spliceDuration_).count(), sendScte35SpliceOutCallback, this);
    }
    else
    {
        g_timeout_add_seconds(std::chrono::seconds(spliceInterval_).count(), sendScte35SpliceInCallback, this);
    }
}

gboolean Pipeline::Impl::pipelineBusWatch(GstBus* /*bus*/, GstMessage* message, gpointer userData)
{
    auto impl = reinterpret_cast<Pipeline::Impl*>(userData);
    impl->onPipelineMessage(message);
    return TRUE;
}

void Pipeline::Impl::demuxPadAddedCallback(GstElement* /*src*/, GstPad* newPad, gpointer userData)
{
    auto impl = reinterpret_cast<Pipeline::Impl*>(userData);
    impl->onDemuxPadAdded(newPad);
}

gboolean Pipeline::Impl::sendScte35SpliceInCallback(gpointer userData)
{
    auto impl = reinterpret_cast<Pipeline::Impl*>(userData);
    impl->sendScte35Splice(SpliceType::IN);
    return FALSE;
}

gboolean Pipeline::Impl::sendScte35SpliceOutCallback(gpointer userData)
{
    auto impl = reinterpret_cast<Pipeline::Impl*>(userData);
    impl->sendScte35Splice(SpliceType::OUT);
    return FALSE;
}

void Pipeline::Impl::run()
{
    if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        Logger::log("Unable to set the pipeline to the playing state.");
        return;
    }
}

void Pipeline::Impl::stop() {}

Pipeline::Pipeline(const std::pair<std::string, uint32_t>& inputAddress,
    const std::pair<std::string, uint32_t>& outputAddress,
    const std::chrono::milliseconds mpegTsBufferSize,
    const std::chrono::minutes spliceInterval,
    const std::chrono::minutes spliceDuration)
    : impl_(std::make_unique<Pipeline::Impl>(inputAddress,
          outputAddress,
          mpegTsBufferSize,
          spliceInterval,
          spliceDuration))
{
}

Pipeline::~Pipeline() // NOLINT(modernize-use-equals-default)
{
}

void Pipeline::run()
{
    impl_->run();
}

void Pipeline::stop()
{
    impl_->stop();
}
