//
// Created by sylar on 25-6-12.
//
#include <QWidget>
#include <QDebug>
#include <gst/app/gstappsink.h>
#include <opencv2/opencv.hpp>
#include "gst_player.h"

std::mutex frame_mutex;
cv::Mat lastest_frame;

GstPlayer::GstPlayer(QObject *) {
    gst_init(nullptr, nullptr);

}

GstPlayer::~GstPlayer() {
    stop();
}

void GstPlayer::play(const std::string &url, WId winId) {
    stop(); // 确保清理旧 pipeline

    pipeline = gst_pipeline_new("rtsp-player");
    auto src = gst_element_factory_make("rtspsrc", "src");
//    auto depay = gst_element_factory_make("rtph264depay", "depay");
//    auto parse = gst_element_factory_make("h264parse", "parse");
//    auto dec = gst_element_factory_make("avdec_h264", "dec");
//    auto conv = gst_element_factory_make("videoconvert", "conv");
//    auto sink = gst_element_factory_make("xvimagesink", "sink");
    auto depay = gst_element_factory_make("rtph264depay", "depay");
    auto parse = gst_element_factory_make("h264parse", "parse");
    auto dec = gst_element_factory_make("vpudec", "dec");
    auto conv = gst_element_factory_make("imxvideoconvert_g2d", "conv");
//    auto sink = gst_element_factory_make("waylandsink", "sink");
    auto sink = gst_element_factory_make("appsink", "sink");

    if (!pipeline || !src || !depay || !parse || !dec || !conv || !sink) {
        g_printerr("Failed to create GStreamer elements\n");
        return;
    }

    g_object_set(sink, "emit-signals", TRUE, "sync" , FALSE, nullptr);

    g_signal_connect(sink, "new-sample", G_CALLBACK(+[](
            GstAppSink* sink, gpointer) -> GstFlowReturn {
        int width;
        int height;
        GstSample* sample = gst_app_sink_pull_sample(sink);
        if (!sample) return GST_FLOW_ERROR;

        GstBuffer* buffer = gst_sample_get_buffer(sample);
        GstCaps* caps = gst_sample_get_caps(sample);
        GstStructure* s = gst_caps_get_structure(caps, 0);
        const gchar* formatName = gst_structure_get_string(s, "format");

//        gchar *caps_str = gst_caps_to_string(caps);

        gst_structure_get_int(s, "width", &width);
        gst_structure_get_int(s, "height", &height);

        g_print("📷 Frame: resolution = %dx%d, buffer size = %" G_GSIZE_FORMAT " bytes ,format = %s \n",
            width, height, gst_buffer_get_size(buffer), formatName);


/*
        GstMapInfo map;
        if(gst_buffer_map(buffer, &map, GST_MAP_READ))
        {
            cv::Mat yuv(height*3/2, width,CV_8UC1,(void*)map.data);
            cv::Mat bgr;
            cv::cvtColor(yuv,bgr,cv::COLOR_YUV2BGR_I420);
            // 示例：输出图像信息
            std::cout << "📷 OpenCV: Got frame " << width << "x" << height
                      << ", type=" << bgr.type()
                      << ", channels=" << bgr.channels() << std::endl;
            {
                std::lock_guard<std::mutex> lock(frame_mutex);
                lastest_frame = bgr.clone();
            }
            gst_buffer_unmap(buffer, &map);
        }*/
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }), nullptr);

    gst_bin_add_many(GST_BIN(pipeline), src, depay, parse, dec, conv, sink, nullptr);

    gst_element_link_many(depay, parse, dec, conv,sink, nullptr);


    GstPad* sinkPad = gst_element_get_static_pad(sink, "sink");
    if (!sinkPad || !gst_pad_is_linked(sinkPad)) {
        qDebug() << "❌ appsink not linked!";
    } else {
        qDebug() << "✅ appsink is linked.";
    }
    gst_object_unref(sinkPad);


    g_signal_connect(src, "pad-added", G_CALLBACK(+[](GstElement *src, GstPad *pad, gpointer data) {
        auto depay = static_cast<GstElement*>(data);
        GstPad *sinkpad = gst_element_get_static_pad(depay, "sink");
        if (!gst_pad_is_linked(sinkpad)) gst_pad_link(pad, sinkpad);
        gst_object_unref(sinkpad);
    }), depay);

    g_object_set(src, "location", url.c_str(), "latency", 150, nullptr);
    gst_element_set_state(sink, GST_STATE_READY);
    if(GST_IS_VIDEO_OVERLAY(sink))
    {
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(sink), (guintptr)winId);
    } else{
        qDebug()<<"sink does not support GstVideoOverlay";
    }
    gst_element_set_state(pipeline, GST_STATE_PLAYING);


    // 添加 bus 监听，输出错误或结束信息
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, [](GstBus* bus, GstMessage* msg, gpointer user_data) -> gboolean {
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR: {
                GError *err;
                gchar *debug;
                gst_message_parse_error(msg, &err, &debug);
                g_printerr("❌ Error: %s\n", err->message);
                g_error_free(err);
                g_free(debug);
                break;
            }
            case GST_MESSAGE_EOS:
                g_print("✅ Stream Ended.\n");
                break;
            case GST_MESSAGE_STATE_CHANGED: {
                GstState old_state, new_state;
                gst_message_parse_state_changed(msg, &old_state, &new_state, nullptr);
                g_print("🔄 State changed from %s to %s\n",
                        gst_element_state_get_name(old_state),
                        gst_element_state_get_name(new_state));
                break;
            }
            case GST_MESSAGE_LATENCY:
                g_print("📶 Latency negotiation triggered\n");
                break;
            case GST_MESSAGE_QOS:
                g_print("⚠️ QoS event received — network/decoding pressure\n");
                break;
            default:
                break;
        }
        return TRUE;
    }, nullptr);
    gst_object_unref(bus);

}

void GstPlayer::stop() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
}

void GstPlayer::pause() {
    if(pipeline)
    {
        gst_element_set_state(pipeline, GST_STATE_PAUSED);
    }
}


