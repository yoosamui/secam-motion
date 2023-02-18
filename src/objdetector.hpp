#pragma once

#include <opencv2/dnn.hpp>
#include <queue>

#include "common.h"
#include "config.h"
#include "constants.h"
#include "ofMain.h"
#include "ofThread.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"
#include "opencv2/imgcodecs.hpp"
#include "videowriter.hpp"

using namespace ofxCv;
using namespace cv;
using namespace std;
using namespace dnn;

class Objectdetector : public ofThread
{
    const uint16_t QUEUE_MAX_SIZE = 5;

    const float INPUT_WIDTH = 640.0;
    const float INPUT_HEIGHT = 640.0;
    const float SCORE_THRESHOLD = 0.2;
    const float NMS_THRESHOLD = 0.4;
    const float CONFIDENCE_THRESHOLD = 0.5;

    const string m_title = "PERSON_";

    struct Detection {
        int class_id;
        float confidence;
        Rect box;
    };
    // clang-format off
    map<string, Scalar> color_map = {
        {"person", Scalar(0, 255, 255)},
        {"motorbike", Scalar(255, 255, 0)},
        {"car", Scalar(0, 255, 0)}
    };
    // clang-format on
    //
    Config &m_config = m_config.getInstance();

    // n    vector<Mat> m_frames;
    vector<string> m_classes;

    int m_frame_number = 1;

    bool m_is_cuda = false;
    bool m_processing = false;
    bool m_detected = false;

    int m_count = 0;

    dnn::Net m_net;

    Rect m_detection_rect;

    string m_filename;
    string m_destination_dir;
    string m_file;
    string m_time_zone = m_config.settings.timezone;

    queue<Mat> m_queue;

    Scalar get_color(const string &name)
    {
        if (color_map.count(name) == 0) return Scalar(255, 255, 255);

        return color_map[name];
    }

    void reset()
    {
        m_detected = false;
        m_processing = false;
    }

    template <typename T>
    string tostr(const T &t, int precision = 2)
    {
        ostringstream ss;
        ss << fixed << setprecision(precision) << t;
        return ss.str();
    }

    Mat format_yolov5(const Mat &source)
    {
        int col = source.cols;
        int row = source.rows;

        int _max = max(col, row);

        Mat result = Mat::zeros(_max, _max, CV_8UC3);

        source.copyTo(result(Rect(0, 0, col, row)));

        return result;
    }

    void threadedFunction()
    {
        vector<Detection> output;
        while (isThreadRunning()) {
            while (!m_queue.empty()  && m_processing ) {
                common::log("Start detection: " + to_string(m_queue.size() ));

                output.clear();
                Mat frame = m_queue.front();
                m_queue.pop();

                m_detected = detect(frame, output);


                if (m_detected) {
                    while (!m_queue.empty()) {
                        m_queue.pop();
                    }

                    break;
                }
            }


            if (m_detected) {
                int value = 0;
                ofNotifyEvent(on_finish_detections, value, this);
                m_detected = false;
                common::log("!!!Person detected!!!");

            }

            m_processing = false;
            ofSleepMillis(10);
        }
    }

    int detect(Mat frame, vector<Detection> &output)

    {
        Mat blob;

        auto input_image = format_yolov5(frame);

        dnn::blobFromImage(input_image, blob, 1. / 255., cv::Size(INPUT_WIDTH, INPUT_HEIGHT),
                           Scalar(), true, false);
        m_net.setInput(blob);
        vector<Mat> outputs;
        m_net.forward(outputs, m_net.getUnconnectedOutLayersNames());

        float x_factor = input_image.cols / INPUT_WIDTH;
        float y_factor = input_image.rows / INPUT_HEIGHT;

        float *data = (float *)outputs[0].data;

        const int rows = 25200;

        vector<int> class_ids;
        vector<float> confidences;
        vector<Rect> boxes;

        for (int i = 0; i < rows; ++i) {
            float confidence = data[4];
            if (confidence >= CONFIDENCE_THRESHOLD) {
                float *classes_scores = data + 5;
                Mat scores(1, m_classes.size(), CV_32FC1, classes_scores);
                Point class_id;
                double max_class_score;
                minMaxLoc(scores, 0, &max_class_score, 0, &class_id);

                if (max_class_score > SCORE_THRESHOLD) {
                    confidences.push_back(confidence);

                    class_ids.push_back(class_id.x);

                    float x = data[0];
                    float y = data[1];
                    float w = data[2];
                    float h = data[3];

                    int left = int((x - 0.5 * w) * x_factor);
                    int top = int((y - 0.5 * h) * y_factor);
                    int width = int(w * x_factor);
                    int height = int(h * y_factor);

                    boxes.push_back(Rect(left, top, width, height));
                }
            }

            data += 85;
        }

        vector<int> nms_result;
        dnn::NMSBoxes(boxes, confidences, SCORE_THRESHOLD, NMS_THRESHOLD, nms_result);
        for (size_t i = 0; i < nms_result.size(); i++) {
            int idx = nms_result[i];

            Detection result;
            result.class_id = class_ids[idx];
            result.confidence = confidences[idx];
            result.box = boxes[idx];

            output.push_back(result);
        }

        return draw(frame, output) != 0;
    }

    bool draw(const Mat &frame, vector<Detection> &output)
    {
        int detections = output.size();
        if (!detections) return false;

        Mat input;
        frame.copyTo(input);

        bool found = false;

        for (int c = 0; c < detections; ++c) {
            auto detection = output[c];

            auto box = detection.box;
            auto classId = detection.class_id;
            auto color = get_color(m_classes[classId]);

            if (m_classes[classId] != "person") continue;

            found = true;

            Rect r = inflate(box, 20, input);

            rectangle(input, r, color, 2);
            rectangle(input, Point(r.x - 1, r.y - 20), cv::Point(r.x + r.width, r.y), color,
                      FILLED);

            float fscale = 0.4;
            int thickness = 1;
            string title = m_classes[classId];

            putText(input, title, Point(r.x + 2, r.y - 5), cv::FONT_HERSHEY_SIMPLEX, fscale,
                    Scalar(0, 0, 0), thickness, LINE_AA, false);
        }

        if (found) imwrite(m_filename, input);
        return found;
    }

    Rect inflate(const Rect &rect, size_t size, const Mat &frame)
    {
        Rect r = rect;

        r.x -= size;
        if (r.x < 0) r.x = 0;

        r.y -= size;
        if (r.y < 0) r.y = 0;

        r.width += size * 2;
        if (r.x + r.width > frame.cols) {
            r.x -= (r.x + r.width) - frame.cols;
        }

        r.height += size * 2;
        if (r.y + r.height > frame.rows) {
            r.y -= (r.y + r.height) - frame.rows;
        }

        return r;
    }

  public:
    ofEvent<int> on_finish_detections;

    Objectdetector()
    {
        ifstream ifs("data/classes.txt");
        string line;
        while (getline(ifs, line)) {
            m_classes.push_back(line);
        }

        auto result = dnn::readNet("data/yolov5s.onnx");

        if (m_is_cuda) {
            result.setPreferableBackend(dnn::DNN_BACKEND_CUDA);
            result.setPreferableTarget(dnn::DNN_TARGET_CUDA_FP16);
        } else {
            result.setPreferableBackend(dnn::DNN_BACKEND_OPENCV);
            result.setPreferableTarget(dnn::DNN_TARGET_CPU);
        }

        m_net = result;
    }

    void setPath()
    {
        m_filename = common::get_filepath("PERSON_" + m_config.parameters.camname, ".jpg", 1);
        m_count = 10;
    }

    void add(const Mat &img)
    {
        if (img.empty() ) {
            return;
        }


        if(m_count++ > 20) return;



        Mat frame;
        img.copyTo(frame);
        common::bgr2rgb(frame);

        m_queue.push(frame);

        common::log("Add probe. " + to_string(m_count));

        //string filename = common::get_filepath("probe", ".jpg", 1);
        //imwrite(filename, frame);

        m_processing = true;
    }
};
