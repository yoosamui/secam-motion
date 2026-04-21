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
