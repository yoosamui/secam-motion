#include "motion_slim.hpp"

MotionSlim::MotionSlim()
{
    // checks detection after 5 secs.
    int frames = (1000 / m_config.parameters.fps) * 5;

    // Set timmers limit values
    //
    m_timex_background.setLimit(frames);

    m_timex_detections.setLimit(frames);
    m_timex_second.setLimit(1000);
    m_timex_recording_point.setLimit(1000);
}

void MotionSlim::init()
{
    m_maskPoints = m_config.mask_points;
    create_mask();
}

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

    detect();
}

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

    Rect max_rect(0, 0, 0, 0);
    bool found = false;

    int w = 0;
    int h = 0;
    m_contour_finder.findContours(m_output);

    // int n = m_contour_finder.getBoundingRects().size();
    // if (n > 4) cout << " ............ " << to_string(n) << endl;

    for (auto &r : m_contour_finder.getBoundingRects())
    {
        if ((r.width > m_config.settings.maxrectwidth ||
             r.height > m_config.settings.maxrectwidth))
        {
            found = false;
            continue;
        }

        if ((r.width > w || r.height > h) && r.width > m_config.settings.minrectwidth &&
            r.height > m_config.settings.minrectwidth)
        {
            max_rect = Rect(r);

            w = r.width;
            h = r.height;

            found = true;
        }
    }

    if (found)
    {
        m_detections_count++;
        ofNotifyEvent(on_motion, max_rect, this);
    }

    if (m_timex_detections.elapsed())
    {
        if (found && m_detections_count >= m_config.settings.detectionsmaxcount &&
            m_contour_finder.size() >= (size_t)m_config.settings.mincontoursize)
        {
            ofNotifyEvent(on_motion_detected, max_rect, this);
        }

        m_detections_count = 0;
        m_timex_detections.set();
    }
}