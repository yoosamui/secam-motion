#include "motion_slim.hpp"

MotionSlim::MotionSlim()
{
    // checks detection after 5 secs.
    // int frames = (1000 / m_config.parameters.fps) * 50;

    // Set timmers limit values
    //
    // m_timex_background.setLimit(frames);

    m_timex_detections.setLimit(2000);
    m_timex_second.setLimit(1000);
    m_timex_recording_point.setLimit(1000);
}

void MotionSlim::init()
{
    m_maskPoints = m_config.mask_points;
    create_mask();
}
void MotionSlim::update(const cv::Mat &frame)
{
    cv::Mat curr_gray, diff, tresh;

    cv::cvtColor(frame, curr_gray, cv::COLOR_BGR2GRAY);
    cv::resize(curr_gray, curr_gray, cv::Size(c_width, c_height));
    // curr_gray.copyTo(m_gray_image, m_mask);
    curr_gray.copyTo(curr_gray, m_mask);

    if (!has_prev)
    {
        curr_gray.copyTo(prev_gray);
        has_prev = true;
        return;
    }

    cv::absdiff(prev_gray, curr_gray, diff);

    cv::threshold(diff, tresh, 25, 255, cv::THRESH_BINARY);
    cv::dilate(tresh, tresh, cv::Mat(), cv::Point(-1, -1), 2);
    // blur(tresh, c_blur);
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(tresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    m_gray_image = tresh;

    int n = contours.size();
    std::cout << "Contours: " << n << std::endl;

    //  cv::Mat output;
    //  diff.copyTo(output); // draw on a copy of original frame
    bool found = false;
    cv::Rect box = cv::Rect(0, 0, 0, 0);

    for (const auto &contour : contours)
    {
        // ignore small noise (important!)
        double area = cv::contourArea(contour);
        if (area < 100)
            continue;

        box = cv::boundingRect(contour);
        std::cout << "box: " << to_string(box.y) << std::endl;
        if (box.y <= 5)
        {

            //   found = false;
            continue;
        }

        // cv::rectangle(output, box, cv::Scalar(0, 255, 0), 2);
        found = true;
    }

    if (found)
    {
        m_detections_count++;
        ofNotifyEvent(on_motion, box, this);
        // ofNotifyEvent(on_motion_detected, box, this);
    }

    if (m_timex_detections.elapsed())
    {

        if (found && m_detections_count >= m_config.settings.detectionsmaxcount)
        {
            std::cout << "-TIME: on_motion_detected" << n << std::endl;
            ofNotifyEvent(on_motion_detected, box, this);
        }

        m_detections_count = 0;
        m_timex_detections.set();
    }

    // update previous frame
    curr_gray.copyTo(prev_gray);
}
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

    /* nuevo */
    cv::Mat prev, curr, diff, tresh;

    convertColor(frame, prev, CV_RGB2GRAY);
    convertColor(frame, curr, CV_RGB2GRAY);

    resize(prev, prev, Size(c_width, c_height));
    resize(curr, curr, Size(c_width, c_height));

    prev.copyTo(prev); //, m_mask);
    curr.copyTo(curr); //, m_mask);

    cv::absdiff(prev, curr, diff);
    cv::threshold(diff, tresh, 25, 255, cv::THRESH_BINARY);

    cv::dilate(tresh, tresh, cv::Mat(), cv::Point(-1, -1), 2);

    // cv::cvtColor(prev, curr, cv::COLOR_BGR2GRAY);

    // frame.copyTo(curr, m_mask);
    // cv::cvtColor(curr, curr, cv::COLOR_BGR2GRAY);

    //  cv::absdiff(prev, curr, diff);
    // cv::threshold(diff, tresh, 25, 255, cv::THRESH_BINARY);

    // cv::dilate(tresh, tresh, cv::Mat(), cv::Point(-1, -1), 2);

    if (tresh.empty())
    {
        cout << " ............ " << to_string(9999) << endl;
        return;
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(tresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    bool found = false;
    int n = contours.size();
    // cout << " ............ " << to_string(n) << endl;
    cout << " ............ " << to_string(n) << endl;

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

    Rect max_rect(0, 0, 0, 0);
    bool found = false;

    int w = 0;
    int h = 0;

    //////////////////////////////////////////////////
    /*     m_contour_finder.findContours(m_output);

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
        } */
}