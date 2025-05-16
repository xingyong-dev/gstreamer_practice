#include <iostream>
#include <gst/gst.h>
#include <string>

class RtspStreamSaver{
public:
    RtspStreamSaver(const std::string& uri, const std::string& output_file)
    : uri_(uri),output_file_(output_file),pipeline_(nullptr){};
    ~RtspStreamSaver(){
        if(pipeline_)
        {
            gst_element_set_state(pipeline_, GST_STATE_NULL);
            gst_object_unref(pipeline_);
        }
    }

    bool initialize()
    {
        gst_init(nullptr, nullptr);
        pipeline_ = gst_pipeline_new("rtsp-pipeline");
        auto rtspsrc = gst_element_factory_make("rtspsrc", "source");
        auto depay = gst_element_factory_make("rstp265depay", "depay");
        auto parser = gst_element_factory_make("h265parse", "parser");
        auto tee = gst_element_factory_make("tee","tee");

        auto queue_display = gst_element_factory_make("queue","queue_display");
        auto decoder = gst_element_factory_make("vpu_h265dec","decoder");
        auto hwconvert = gst_element_factory_make("imxvideoconvert_g2d", "hwconvert");
        auto sink = gst_element_factory_make("waylandsink","sink");

        auto queue_save = gst_element_factory_make("queue", "queue_save");
        auto mux = gst_element_factory_make("mp4mux","muxer");
        auto filesink = gst_element_factory_make("filesink", "filesink");

        if(!pipeline_ || !rtspsrc || !depay || !parser || !tee ||
            !queue_display || !decoder || !hwconvert || !sink ||
            !queue_save || !mux || !filesink)
        {
            std::cout<<"failed to create GSTreamer elelemnts "<< std::endl;
            return false;
        }
        g_object_set(rtspsrc ,"location", uri_.c_str(), "latency" , 0, nullptr);
        g_object_set(filesink, "location", output_file_.c_str(), nullptr);

        gst_bin_add_many(GST_BIN(pipeline_), rtspsrc, depay, parser, tee,
                         queue_display, decoder, hwconvert, sink,
                         queue_save, mux, filesink, nullptr);
        g_signal_connect(rtspsrc,"pad-added", G_CALLBACK(+[](GstElement* src, GstPad* pad, gpointer data){
            auto depay = static_cast<GstElement *>(data);
            auto sinkpad = gst_element_get_static_pad(depay, "sink");
            gst_pad_link(pad, sinkpad);
            gst_object_unref(sinkpad);
        }), depay);

        if(!gst_element_link_many(depay, parser, tee, nullptr) ||
                !gst_element_link_many(tee,queue_display, decoder, hwconvert, sink, nullptr) ||
                !gst_element_link_many(tee, queue_save, mux, filesink, nullptr))
        {
            std::cerr << "Failed to link elements" << std::endl;
            return false;
        }
        return true;

    }

    void run()
    {
        gst_element_set_state(pipeline_, GST_STATE_PLAYING);
        GstBus *bus = gst_element_get_bus(pipeline_);
        GstMessage * msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

        if(msg)
        {
            GError * err = nullptr;
            gchar * debug = nullptr;
            gst_message_parse_error(msg, &err, &debug);
            std::cerr << "GStreamer Error: " << err->message << std::endl;
            g_error_free(err);
            g_free(debug);
            gst_message_unref(msg);
        }
        gst_object_unref(bus);
    }


private:
    const std::string &uri_;
    const std::string &output_file_;
    GstElement* pipeline_;
};
int main() {
    RtspStreamSaver saver("rtsp://admin:xingyong1234@192.168.2.115:1234", "output.mp4");
    if(saver.initialize())
    {
        saver.run();
    }
    return 0;
}
