import gi
gi.require_version('Gst', '1.0')
gi.require_version('GstVideo', '1.0')
from gi.repository import Gst, GLib
import cv2
import numpy as np
import tensorflow as tf
import time

# 初始化 GStreamer
Gst.init(None)

RTSP_URL = "rtsp://admin:12345@192.168.0.200:554/Streaming/Channels/102"
MODEL_PATH = "detect.tflite"
LABEL_PATH = "labelmap.txt"
FRAME_WIDTH, FRAME_HEIGHT = 300, 300  # 模型输入分辨率

def load_labels(path):
    with open(path, 'r') as f:
        return [line.strip() for line in f.readlines()]

def setup_pipeline():
    pipeline_str = (
        f"rtspsrc location={RTSP_URL} latency=100 ! "
        f"rtph264depay ! h264parse ! vpudec ! "
        f"imxvideoconvert_g2d ! video/x-raw,format=RGB,width={FRAME_WIDTH},height={FRAME_HEIGHT} ! "
        "appsink name=sink emit-signals=true max-buffers=1 drop=true"
    )
    pipeline = Gst.parse_launch(pipeline_str)
    sink = pipeline.get_by_name("sink")
    return pipeline, sink

def gst_to_opencv(sample):
    buf = sample.get_buffer()
    caps = sample.get_caps()
    arr = np.ndarray(
        (
            caps.get_structure(0).get_value('height'),
            caps.get_structure(0).get_value('width'),
            3
        ),
        buffer=buf.extract_dup(0, buf.get_size()),
        dtype=np.uint8
    )
    return arr

def main():
    labels = load_labels(LABEL_PATH)

    delegate = tf.lite.experimental.load_delegate("libvx_delegate.so")
    interpreter = tf.lite.Interpreter(model_path=MODEL_PATH, experimental_delegates=[delegate])
    interpreter.allocate_tensors()

    input_index = interpreter.get_input_details()[0]['index']
    output_details = interpreter.get_output_details()

    pipeline, sink = setup_pipeline()
    pipeline.set_state(Gst.State.PLAYING)

    def on_new_sample(sink):
        sample = sink.emit("pull-sample")
        frame = gst_to_opencv(sample)
        input_tensor = np.expand_dims(frame, axis=0).astype(np.uint8)
        interpreter.set_tensor(input_index, input_tensor)
        interpreter.invoke()

        boxes = interpreter.get_tensor(output_details[0]['index'])[0]
        classes = interpreter.get_tensor(output_details[1]['index'])[0]
        scores = interpreter.get_tensor(output_details[2]['index'])[0]

        for i in range(len(scores)):
            if scores[i] > 0.5:
                y1, x1, y2, x2 = boxes[i]
                x1 = int(x1 * FRAME_WIDTH)
                x2 = int(x2 * FRAME_WIDTH)
                y1 = int(y1 * FRAME_HEIGHT)
                y2 = int(y2 * FRAME_HEIGHT)
                label = labels[int(classes[i])]
                cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                cv2.putText(frame, f"{label}: {scores[i]:.2f}", (x1, y1 - 5),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

        cv2.imshow("Detection", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            loop.quit()
        return Gst.FlowReturn.OK

    sink.connect("new-sample", on_new_sample)

    loop = GLib.MainLoop()
    try:
        loop.run()
    except KeyboardInterrupt:
        pass
    pipeline.set_state(Gst.State.NULL)

if __name__ == "__main__":
    main()

