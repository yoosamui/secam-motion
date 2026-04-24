#include "motion_slim.hpp"

MotionSlim::MotionSlim()
{
    // checks detection after 5 secs.
    // int frames = (1000 / m_config.parameters.fps) * 50;

    // Set timmers limit values
    //
    // m_timex_background.setLimit(frames);

    m_timex_detections.setLimit(1600);
    m_timex_second.setLimit(1000);
    m_timex_recording_point.setLimit(1000);
}

void MotionSlim::init()
{
    m_maskPoints = m_config.mask_points;
    create_mask();
}
// #define UNUSED_OPTI_BOX 1
#ifdef UNUSED_OPTI_BOX
void MotionSlim::update(const cv::Mat &frame)
{
    if (frame.empty())
        return;

    // --- Step 1: Convert to grayscale and resize (optimized) ---
    cv::Mat currGray;

    // Use direct grayscale conversion if input is BGR
    if (frame.type() == CV_8UC3)
    {
        cv::cvtColor(frame, currGray, cv::COLOR_BGR2GRAY);
    }
    else if (frame.type() == CV_8UC1)
    {
        currGray = frame; // Already grayscale, avoid copy
    }
    else
    {
        return; // Unsupported format
    }

    // Fast resize only when needed
    if (currGray.cols != c_width || currGray.rows != c_height)
    {
        cv::resize(currGray, currGray, cv::Size(c_width, c_height), 0, 0, cv::INTER_NEAREST);
    }

    // --- Step 2: Apply mask (ROI) - optimized ---
    if (cv::countNonZero(m_mask) < (c_width * c_height) * 0.3)
    {
        currGray.setTo(0, ~m_mask);
    }
    else
    {
        currGray.copyTo(currGray, m_mask);
    }

    // --- Step 3: Initialize previous frame ---
    if (!has_prev)
    {
        currGray.copyTo(prev_gray);
        has_prev = true;
        return;
    }

    // --- Step 4: Frame difference ---
    cv::Mat diff;
    cv::absdiff(prev_gray, currGray, diff);

    // --- Step 5: Threshold + dilation ---
    cv::Mat thresh;
    cv::threshold(diff, thresh, 25, 255, cv::THRESH_BINARY);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(thresh, thresh, kernel, cv::Point(-1, -1), 1); // Reduced to 1 iteration

    // --- Step 6: Find contours ---
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    m_gray_image = currGray;
    m_output = thresh;
    m_detected_Polylines.clear();

    // --- Step 7: Collect valid detections ---
    std::vector<cv::Rect> boxes;
    boxes.reserve(contours.size());

    const float minArea = 100.0f;
    const int topMargin = 20;

    for (const auto &contour : contours)
    {
        cv::Rect box = cv::boundingRect(contour);

        if (box.y <= topMargin || box.width < 5 || box.height < 5)
            continue;

        float approxArea = static_cast<float>(box.width * box.height);
        if (approxArea < minArea)
            continue;

        boxes.push_back(box);
    }

    bool foundMotion = !boxes.empty();

    // --- Step 8: Emit per-frame motion event ---
    if (foundMotion)
    {
        m_detections_count += boxes.size();
        ofNotifyEvent(on_motion, boxes, this);
    }

    // --- Step 9: Time-based aggregation ---
    if (m_timex_detections.elapsed())
    {
        if (foundMotion && m_detections_count >= m_config.settings.detectionsmaxcount)
        {
            ofNotifyEvent(on_motion_detected, boxes, this);
        }
        m_detections_count = 0;
        m_timex_detections.reset();
    }

    // --- Step 10: Update previous frame (fixed) ---
    currGray.copyTo(prev_gray); // Use copyTo instead of swap
}
#endif

#define UNUSED_MULTIPLE_BOX 1
#ifdef UNUSED_MULTIPLE_BOX
void MotionSlim::update(const cv::Mat &frame)
{
    if (frame.empty())
        return;

    // --- Step 1: Convert to grayscale and resize ---
    cv::Mat currGray;
    cv::cvtColor(frame, currGray, cv::COLOR_BGR2GRAY);

    // Resize with INTER_NEAREST for speed (good enough for motion detection)
    if (currGray.cols != c_width || currGray.rows != c_height)
        cv::resize(currGray, currGray, cv::Size(c_width, c_height), 0, 0, cv::INTER_NEAREST);

    // --- Step 2: Apply mask (ROI) ---
    currGray.setTo(0, ~m_mask);

    // --- Step 3: Initialize previous frame ---
    if (!has_prev)
    {
        currGray.copyTo(prev_gray);
        has_prev = true;
        return;
    }

    // --- Step 4: Frame difference ---
    cv::Mat diff;
    cv::absdiff(prev_gray, currGray, diff);

    // --- Step 5: Threshold + dilation ---
    cv::Mat thresh;
    cv::threshold(diff, thresh, 25, 255, cv::THRESH_BINARY);
    cv::dilate(thresh, thresh, cv::Mat(), cv::Point(-1, -1), 2);

    // --- Step 6: Find contours ---
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    m_gray_image = currGray;
    m_output = thresh;
    m_detected_Polylines.clear();
    // --- Step 7: Collect valid detections ---
    std::vector<cv::Rect> boxes;

    for (const auto &contour : contours)
    {
        double area = cv::contourArea(contour);

        // Ignore noise
        if (area < 100.0)
            continue;

        cv::Rect box = cv::boundingRect(contour);

        // Ignore top region (custom rule)
        if (box.y <= 20)
            continue;

        boxes.push_back(box);
    }

    bool foundMotion = !boxes.empty();

    // --- Step 8: Emit per-frame motion event ---
    if (foundMotion)
    {
        m_detections_count += boxes.size();    // count all detections
        ofNotifyEvent(on_motion, boxes, this); // <-- vector instead of single box
    }

    // --- Step 9: Time-based aggregation ---
    if (m_timex_detections.elapsed())
    {
        if (foundMotion &&
            m_detections_count >= m_config.settings.detectionsmaxcount)
        {
            ofNotifyEvent(on_motion_detected, boxes, this);
        }

        m_detections_count = 0;
        m_timex_detections.reset();
    }

    // --- Step 10: Update previous frame ---
    currGray.copyTo(prev_gray);
}
#endif
#ifdef UNUSED_ONE_BOX
void MotionSlim::update(const cv::Mat &frame)
{
    // --- Step 1: Convert input frame to grayscale and resize ---
    cv::Mat currGray;
    cv::cvtColor(frame, currGray, cv::COLOR_BGR2GRAY);
    cv::resize(currGray, currGray, cv::Size(c_width, c_height));

    // --- Step 2: Apply polygon mask (keep only ROI, zero everything else) ---
    // Assumes m_mask is CV_8UC1 with same size as currGray
    currGray.setTo(0, ~m_mask);

    // --- Step 3: Initialize previous frame (first call only) ---
    if (!has_prev)
    {
        currGray.copyTo(prev_gray);
        has_prev = true;
        return;
    }

    // --- Step 4: Compute frame difference ---
    cv::Mat diff;
    cv::absdiff(prev_gray, currGray, diff);

    // --- Step 5: Threshold to get motion regions ---
    cv::Mat thresh;
    cv::threshold(diff, thresh, 25, 255, cv::THRESH_BINARY);

    // --- Step 6: Dilate to fill gaps and merge nearby regions ---
    cv::dilate(thresh, thresh, cv::Mat(), cv::Point(-1, -1), 2);

    // --- Step 7: Find contours (motion blobs) ---
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Store current grayscale image (for debugging/visualization)
    m_gray_image = currGray;

    // --- Step 8: Process contours ---
    bool foundMotion = false;
    cv::Rect detectedBox;

    for (const auto &contour : contours)
    {
        double area = cv::contourArea(contour);

        // Ignore small noise
        if (area < 100.0)
            continue;

        cv::Rect box = cv::boundingRect(contour);

        // Ignore detections too close to top edge (custom rule)
        if (box.y <= 20)
            continue;

        detectedBox = box;
        foundMotion = true;

        // Optional: break if you only care about the first valid detection
        // break;
    }

    // --- Step 9: Emit motion event (per frame) ---
    if (foundMotion)
    {
        m_detections_count++;
        ofNotifyEvent(on_motion, detectedBox, this);
    }

    // --- Step 10: Time-based aggregation of detections ---
    if (m_timex_detections.elapsed())
    {
        if (foundMotion &&
            m_detections_count >= m_config.settings.detectionsmaxcount)
        {
            ofNotifyEvent(on_motion_detected, detectedBox, this);
        }

        // Reset counter and timer
        m_detections_count = 0;
        m_timex_detections.reset();
    }

    // --- Step 11: Update previous frame ---
    currGray.copyTo(prev_gray);
}
#endif

#ifdef UNUSED
void MotionSlim::update(const Mat &frame)
{

    /*  convertColor(frame, m_gray, CV_RGB2GRAY);
     resize(m_gray, m_gray, Size(c_width, c_height));

     m_gray.copyTo(m_gray_image, m_mask);
     blur(m_gray, c_blur);

     m_gray.copyTo(m_mask_image, m_mask);
     m_mog2->apply(m_mask_image, m_difference);

     // threshold shadows
     threshold(m_difference, m_output, c_threshold, 255, CV_THRESH_BINARY); */

    // detect();
}
#endif

void MotionSlim::create_mask()
{
    if (m_maskPoints.size() == 0)
    {
        m_maskPoints.push_back(cv::Point(1, 1));
        m_maskPoints.push_back(cv::Point(c_width - 1, 1));
        m_maskPoints.push_back(cv::Point(c_width - 1, c_height - 1));
        m_maskPoints.push_back(cv::Point(1, c_height - 1));
        m_maskPoints.push_back(cv::Point(1, 1));
    }

    CvMat *matrix = cvCreateMat(c_height, c_width, CV_8UC1);
    m_mask = cvarrToMat(matrix);

    for (int x = 0; x < m_mask.cols; x++)
    {
        for (int y = 0; y < m_mask.rows; y++)
            m_mask.at<uchar>(cv::Point(x, y)) = 0;
    }

    fillPoly(m_mask, m_maskPoints, 255);

    // Notify client
    ofNotifyEvent(on_mask_updated, this);
}

void MotionSlim::resetMask()
{
    m_maskPoints.clear();
    m_polyline.clear();
}

void MotionSlim::detect()
{
    m_found_motion.clear();
    m_detected.clear();
}