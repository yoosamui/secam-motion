#pragma once

#include "common.h"
#include "config.h"
#include "constants.h"
#include "ofxOpenCv.h"
#include <vector>

class IMotionSlim
{
public:
    virtual void init() = 0;
    virtual void update(const Mat &frame) = 0;
    virtual void detect() = 0;
};

/// @brief Motion detetion occurs here
class MotionSlim //: public IMotionSlim
{

    // Optional: Add configuration struct to header file
    struct MotionConfig
    {
        // Threshold and filtering
        int motion_threshold = 25;  // Lower = more sensitive
        int min_area = 100;         // Minimum area to consider as motion
        int max_area = 1000000;     // Maximum area (ignore huge regions)
        int dilate_iterations = 2;  // Dilation iterations
        int ignore_top_region = 20; // Pixels to ignore from top
        int ignore_edge = 0;        // Pixels to ignore from edges

        // Optimization flags
        bool remove_noise = true;            // Apply erosion after dilation
        bool use_temporal_smoothing = false; // Smooth between frames
        bool use_frame_skipping = false;     // Skip frames for performance
        int frame_skip_counter = 0;          // Current frame skip count

        // Detection settings
        struct Settings
        {
            int detectionsmaxcount = 3; // Minimum detections to trigger event
        } settings;
    };

    // Optimization parameters
    const int PROCESS_WIDTH = 160; // Tiny resolution
    const int PROCESS_HEIGHT = 120;
    const int FRAME_SKIP = 2;         // Process every 2nd frame
    const int MOTION_THRESHOLD = 30;  // Higher = less sensitive but faster
    const int MIN_MOTION_PIXELS = 50; // Minimum pixels to trigger motion

    cv::Mat prevGray;
    // bool has_prev = false;
    int frameCount = 0;

    // Performance monitoring
    std::chrono::steady_clock::time_point lastTime;
    float fps = 0;

    cv::Mat prev_gray;
    bool has_prev = false;

    const int c_width = 320;
    const int c_height = 240;
    int m_detections_count = 0;

    bool m_first_set = false;

    // bool m_motion_detected = false;

    Mat m_mask;
    Mat m_mask_image;
    //  Mat m_resized;
    Mat m_gray;
    Mat m_gray_image;
    Mat m_output;
    Mat m_threshold;
    Mat m_difference;

    Mat m_first;
    Mat m_second;

    ofPolyline m_found_motion;
    ofPolyline m_detected;
    ofPolyline m_polyline;
    ofPolyline m_maskPolyLineScaled;

    vector<Point> m_maskPoints;

    Config &m_config = m_config.getInstance();
    MotionConfig m_motion_config;
    // Ptr<cv::BackgroundSubtractorMOG2> m_mog2 = createBackgroundSubtractorMOG2();
    ContourFinder m_contour_finder;

    common::Timex m_timex_detections;
    common::Timex m_timex_stoprecording;
    common::Timex m_timex_second;
    common::Timex m_timex_recording_point;
    common::Timex m_timex_background;

public:
    vector<ofPolyline> m_detected_Polylines;
    // ofEvent<Rect> on_motion;
    // ofEvent<Rect> on_motion_detected;
    ofEvent<void> on_mask_updated;

    ofEvent<std::vector<cv::Rect>> on_motion;
    ofEvent<std::vector<cv::Rect>> on_motion_detected;

public:
    MotionSlim();

    virtual void init();
    virtual void detect();
    virtual void update(const Mat &frame);

    void create_mask();
    void resetMask();

    int getWidth() const { return c_width; }
    int getHeight() const { return c_height; }

    cv::Mat &getGrayImage() { return m_gray_image; }
    cv::Mat &getOutputImage() { return m_output; }

    const vector<Point> &getMaskPoints() const { return m_maskPoints; }
};
