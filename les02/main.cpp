#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <opencv2/opencv.hpp>
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/model.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <dlfcn.h>
#include <unistd.h>
#include <memory>
#include <map>

#define MODEL_PATH "ssd_mobilenet_v2.tflite"
#define LABEL_PATH "coco_labels.txt"
#define RTSP_URL   "rtsp://admin:12345@192.168.0.200:554/Streaming/Channels/102"

std::vector<std::string> LoadLabels(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<std::string> labels;
    std::string line;
    while (std::getline(file, line)) {
        labels.push_back(line);
    }
    return labels;
}

class DelegateWrapper {
public:
    DelegateWrapper(const std::string& lib_path,
                    const std::map<std::string, std::string>& options) {
        handle_ = dlopen(lib_path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle_) {
            std::cerr << "[ERROR] Failed to load delegate: " << lib_path << std::endl;
            return;
        }

        using CreateDelegateFn = TfLiteDelegate* (*)(char**, char**, size_t);
        CreateDelegateFn create = (CreateDelegateFn)dlsym(handle_, "tflite_plugin_create_delegate");
        if (!create) {
            std::cerr << "[ERROR] Symbol not found: tflite_plugin_create_delegate" << std::endl;
            return;
        }

        for (const auto& kv : options) {
            if (kv.first.empty() || kv.second.empty()) continue;
            keys_.push_back(strdup(kv.first.c_str()));
            values_.push_back(strdup(kv.second.c_str()));
        }

        delegate_ = create(keys_.data(), values_.data(), keys_.size());
        if (!delegate_) {
            std::cerr << "[ERROR] Delegate creation failed\n";
        }
    }

    TfLiteDelegate* get() const { return delegate_; }

    ~DelegateWrapper() {
        if (handle_) dlclose(handle_);
        for (auto ptr : keys_) free(ptr);
        for (auto ptr : values_) free(ptr);
    }

private:
    void* handle_ = nullptr;
    TfLiteDelegate* delegate_ = nullptr;
    std::vector<char*> keys_;
    std::vector<char*> values_;
};

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);

    std::string pipeline_str =
            "rtspsrc location=" RTSP_URL " latency=200 ! "
            "rtph264depay ! h264parse ! vpudec ! "
            "imxvideoconvert_g2d ! video/x-raw,format=RGB ! "
            "appsink name=sink max-buffers=1 drop=true emit-signals=true sync=false";
//    std::string pipeline_str =
//            "videotestsrc is-live=true ! "
//            "video/x-raw,width=320,height=240,format=RGB ! "
//            "appsink name=sink max-buffers=1 drop=true emit-signals=true sync=false";

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    auto model = tflite::FlatBufferModel::BuildFromFile(MODEL_PATH);
    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;
    tflite::InterpreterBuilder(*model, resolver)(&interpreter);

    DelegateWrapper vx_delegate("libvx_delegate.so", {});
    if (vx_delegate.get()) {
        interpreter->ModifyGraphWithDelegate(vx_delegate.get());
        std::cout << "[INFO] Using VX Delegate for acceleration.\n";
    } else {
        std::cout << "[WARN] VX Delegate unavailable, fallback to CPU.\n";
    }

    interpreter->AllocateTensors();

    auto input_tensor = interpreter->typed_input_tensor<uint8_t>(0);
    auto labels = LoadLabels(LABEL_PATH);

    int input_w = interpreter->tensor(interpreter->inputs()[0])->dims->data[2];
    int input_h = interpreter->tensor(interpreter->inputs()[0])->dims->data[1];

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    static auto last_save_time = std::chrono::steady_clock::now();
    static int save_index = 1;
    const int save_interval_sec = 10;
    const int max_save_count = 10;

    static int frame_count = 0;
    static auto last_fps_time = std::chrono::steady_clock::now();
    while (true) {
        GstSample* sample = nullptr;
        std::cout << "[test] ~~~~~~~~~~~~~~~~~~~~~~~3~~~~~~~~~~~~~~~~~~~~~~~~~~.\n";
        g_signal_emit_by_name(sink, "pull-sample", &sample);
        if (!sample) {
            usleep(500000);
            continue;
        }
        std::cout << "[test] ~~~~~~~~~~~~~~~~~~~~~~~4~~~~~~~~~~~~~~~~~~~~~~~~~~.\n";
        GstBuffer* buffer = gst_sample_get_buffer(sample);
        GstCaps* caps = gst_sample_get_caps(sample);
        GstMapInfo map;
        gst_buffer_map(buffer, &map, GST_MAP_READ);

        GstStructure* s = gst_caps_get_structure(caps, 0);
        int width, height;
        gst_structure_get_int(s, "width", &width);
        gst_structure_get_int(s, "height", &height);

        cv::Mat frame(height, width, CV_8UC3, (char*)map.data);
        cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);

        cv::Mat resized;
        cv::resize(frame, resized, cv::Size(input_w, input_h));
        cv::Mat input_img;
        resized.convertTo(input_img, CV_8UC3);
        memcpy(input_tensor, input_img.data, input_img.total() * input_img.elemSize());
        std::cout << "[test] ~~~~~~~~~~~~~~~~~~~~~~~5~~~~~~~~~~~~~~~~~~~~~~~~~~.\n";
        auto t0 = std::chrono::high_resolution_clock::now();
        TfLiteStatus status = interpreter->Invoke();
        auto t1 = std::chrono::high_resolution_clock::now();

        if (status != kTfLiteOk) {
            std::cerr << "[ERROR] Inference failed!" << std::endl;
            break;
        }
        std::cout << "[test] ~~~~~~~~~~~~~~~~~~~~~~~6~~~~~~~~~~~~~~~~~~~~~~~~~~.\n";
        float inference_time = std::chrono::duration<float, std::milli>(t1 - t0).count();
        ++frame_count;
        auto now =std::chrono::steady_clock::now();
        float elapsed_sec = std::chrono::duration<float>(now - last_fps_time).count();

        if (elapsed_sec >= 1.0f) {
            float fps = frame_count / elapsed_sec;
            std::cout << "[PERF] Inference time: " << inference_time << " ms | FPS: " << fps << std::endl;
            frame_count = 0;
            last_fps_time = now;
        }
        std::cout << "[test] ~~~~~~~~~~~~~~~~~~~~~~~7~~~~~~~~~~~~~~~~~~~~~~~~~~.\n";
        float* boxes = interpreter->typed_output_tensor<float>(0);
        float* classes = interpreter->typed_output_tensor<float>(1);
        float* scores = interpreter->typed_output_tensor<float>(2);
        int* num = interpreter->typed_output_tensor<int>(3);

        for (int i = 0; i < num[0]; ++i) {
            float score = scores[i];
            if (score < 0.5f) continue;

            int class_id = static_cast<int>(classes[i]);
            std::string label = labels[class_id];

            float y1 = boxes[4 * i] * height;
            float x1 = boxes[4 * i + 1] * width;
            float y2 = boxes[4 * i + 2] * height;
            float x2 = boxes[4 * i + 3] * width;

            std::cout << "[DETECT] " << label << " " << int(score * 100)
                      << "% box=(" << int(x1) << "," << int(y1) << ")-(" << int(x2) << "," << int(y2) << ")\n";
        }
        std::cout << "[test] ~~~~~~~~~~~~~~~~~~~~~~~8~~~~~~~~~~~~~~~~~~~~~~~~~~.\n";
        float time_since_last_save = std::chrono::duration<float>(now - last_save_time).count();
        if (time_since_last_save >= save_interval_sec && save_index <= max_save_count) {
            std::ostringstream filename;
            filename << "frame_" << std::setfill('0') << std::setw(3) << save_index++ << ".jpg";
            if (cv::imwrite(filename.str(), frame)) {
                std::cout << "[SAVE] Saved frame to " << filename.str() << std::endl;
            } else {
                std::cerr << "[ERROR] Failed to save frame: " << filename.str() << std::endl;
            }
            last_save_time = now;
        }
        std::cout << "[test] ~~~~~~~~~~~~~~~~~~~~~~~9~~~~~~~~~~~~~~~~~~~~~~~~~~.\n";
        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
