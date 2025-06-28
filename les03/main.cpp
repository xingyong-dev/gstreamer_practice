#include <iostream>
extern "C"
{
    #include <gst/gst.h>
}


#define uri "rtsp://admin:12345@192.168.0.200:554/Streaming/Channels/102"


int main(int argc, char * argv[]) {

    gst_init(&argc , &argv);

    GstElement * pipeline = gst_pipeline_new("dog-player");
    GstElement * source   = gst_element_factory_make("rtspsrc", "videosrc");  //去掉帧头帧尾，只保留h264格式；
    GstElement * depay   = gst_element_factory_make("rtph264depay", "depay");
    GstElement * parser    = gst_element_factory_make("h264parse", "parser");
    GstElement * decoder   = gst_element_factory_make("avdec_h264","decoder");
    GstElement * convert   = gst_element_factory_make("videoconvert","convert");
    GstElement * sink      = gst_element_factory_make("autovideosink", "sink");

    if(!pipeline || !source || !depay || !parser || !decoder || !convert || !sink )
    {
        g_printerr("Failed to create elements\n");
        return -1;
    }

    g_object_set(source, "location", uri ,"latency", 100,NULL);

    gst_bin_add_many(GST_BIN(pipeline),depay,parser,decoder,convert,sink, NULL);

    g_signal_connect(source , "pad_added", G_CALLBACK(+[](GstElement* src, GstPad *pad, gpointer user_data){
            GstElement * depay = static_cast<GstElement *>(user_data);
            GstPad *sinkpad = gst_element_get_static_pad(depay, "sink");
            if(!gst_pad_is_linked(sinkpad))
            {
                GstCaps *caps = gst_pad_get_current_caps(pad);
                if(caps){
                    const gchar *name = gst_structure_get_name(gst_caps_get_structure(caps, 0));
                    if(g_str_has_prefix(name, "application/x-rtp"))
                    {
                        gst_pad_link(pad, sinkpad);
                    }
                    gst_caps_unref(caps);
                }
            }
            gst_object_unref(sinkpad);
        }), depay);
    gst_bin_add(GST_BIN(pipeline), source);

    if(!gst_element_link_many(depay, parser, decoder, convert, sink, NULL)){
        g_printerr("Failed to link elements.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to start pipeline.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    // 监听消息总线
    GstBus *bus = gst_element_get_bus(pipeline);
    bool terminate = false;
    while (!terminate) {
        GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                                     GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

        if (msg != nullptr) {
            GError *err;
            gchar *debug_info;

            switch (GST_MESSAGE_TYPE(msg)) {
                case GST_MESSAGE_ERROR:
                    gst_message_parse_error(msg, &err, &debug_info);
                    g_printerr("Error: %s\n", err->message);
                    g_error_free(err);
                    g_free(debug_info);
                    terminate = true;
                    break;

                case GST_MESSAGE_EOS:
                    g_print("End of stream\n");
                    terminate = true;
                    break;

                default:
                    break;
            }
            gst_message_unref(msg);
        }
    }

    // 清理
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
