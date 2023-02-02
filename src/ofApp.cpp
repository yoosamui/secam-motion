#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
    ofSetFrameRate(m_config.parameters.fps);
    ofSetVerticalSync(true);

    m_width = m_config.parameters.width;
    m_height = m_config.parameters.height;

    ofLog::setAutoSpace(false);

    if (!m_config.load()) {
        common::log("load Configuration error.", OF_LOG_ERROR);
        terminate();
    }

    stringstream ss;
    ss << "Configuration:\n"
       << "uri = " << m_config.settings.uri << "\n"
       << "cam = " << m_config.parameters.camname << "\n"
       << "timezone = " << m_config.settings.timezone << "\n"
       << "storage = " << m_config.settings.storage << "\n"
       << "minarearadius = " << m_config.settings.minarearadius << "\n"
       << "minrectwidth = " << m_config.settings.minrectwidth << "\n"
       << "minthreshold = " << m_config.settings.minthreshold << "\n"
       << "mincontousize =  " << m_config.settings.mincontoursize << "\n"
       << "detectionsmaxcount = " << m_config.settings.detectionsmaxcount << "\n"
       << endl;

    common::log(ss.str());

    if (!m_config.isServer()) {
        ofSetWindowTitle("CAM-" + m_config.parameters.camname + " / " + m_config.settings.timezone);
        m_font.load(OF_TTF_SANS, 9, true, true);
    }

    ofAddListener(m_motion.on_motion, this, &ofApp::on_motion);
    ofAddListener(m_motion.on_motion_detected, this, &ofApp::on_motion_detected);
    ofAddListener(m_motion.on_mask_updated, this, &ofApp::on_mask_updated);

    m_motion.init();

    m_timex_stoprecording.setLimit(c_videoduration * 1000);
    m_timex_second.setLimit(1000);
    m_timex_recording_point.setLimit(1000);

    m_processing = true;
}

//--------------------------------------------------------------
void ofApp::ofExit()
{
    m_cmd.startThread();
    m_cmd.ffmpeg_stop();

    cout << "exit" << endl;
}

//--------------------------------------------------------------
void ofApp::update()
{
    if (!m_cam.read(m_frame)) {
        cout << "Disconnected" << endl;

        m_cam.release();
        ofSleepMillis(500);

        // m_cam.set(CAP_PROP_CONVERT_RGB, true); //dont work
        if (m_cam.open(m_config.settings.uri, CAP_FFMPEG)) {
            cout << "Connected" << endl;
        }
        return;
    }

    if (m_frame.empty()) {
        return;
    }

    if (m_framecount++ > 1000000) m_framecount = 0;
    if (!m_processing) return;

    // Processing
    common::bgr2rgb(m_frame);

    // Write the timestam
    auto timestamp = common::getTimestampMillis("Asia/Bangkok", "%Y.%m.%d %T");
    int fontface = cv::FONT_HERSHEY_SIMPLEX;
    double scale = 0.8;
    int thickness = 1.7;
    int x = m_frame.cols;
    int y = 0;

    cv::rectangle(m_frame, Point(x, y + 1), Point(x - 350, 30), CV_RGB(0, 0, 0), CV_FILLED);
    cv::putText(m_frame, timestamp, cv::Point(x - 330, y + 24), fontface, scale,
                cv::Scalar(255, 255, 255), thickness, LINE_AA, false);

    // TODO use ffmpeg for write video
    // add frame to writer
    // m_writer.add(m_frame);

    cv::resize(m_frame, m_resized, cv::Size(m_width, m_height));

    m_found_motion.clear();
    m_detected.clear();
    m_max_rect = Rect(0, 0, 0, 0);

    // Motion detector
    m_motion.update(m_frame);

    if (m_motion_detected) {
        if (!m_recording) {
            m_recording = true;

            saveDetectionImage();

            m_cmd.startThread();
            m_cmd.ffmpeg_start();
            common::log("Recording...");

            ofResetElapsedTimeCounter();
        }

        m_recording_duration = c_videoduration;
        m_timex_stoprecording.reset();

        m_motion_detected = false;
    }

    if (m_recording) {
        if (m_timex_second.elapsed()) {
            m_recording_duration--;
            m_timex_second.set();
        }

        if (m_timex_stoprecording.elapsed()) {
            m_cmd.stopThread();
            m_cmd.ffmpeg_stop();

            common::log("Recording finish.");
            // not procesing util detector finish.
            //////////   m_processing = false;
            //////     m_detector.detect();

            m_recording_duration = c_videoduration;
            m_recording = false;

            m_timex_stoprecording.set();
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw()
{
    if (m_config.isServer()) return;

    ofBackground(ofColor::black);

    // draw mask polylines
    if (m_view == 2) {
        cv::polylines(m_motion.getGrayImage(), m_motion.getMaskPoints(), true,
                      Scalar(255, 255, 255), 1, 8);
        cv::polylines(m_motion.getOutputImage(), m_motion.getMaskPoints(), true,
                      Scalar(255, 255, 255), 1, 8);
    }

    switch (m_view) {
        case 0:
        case 1:
            drawMat(m_resized, 0, 0);
            break;
        case 2:
            drawMat(m_motion.getGrayImage(), 0, 0);
            drawMat(m_motion.getOutputImage(), m_motion.getOutputImage().cols, 0);
            break;
        default:
            return;
    };

    // Draw yellow
    ofPushStyle();
    ofNoFill();
    ofSetLineWidth(1.0);
    ofSetColor(yellowPrint);

    if (m_view == 1) {
        // Draw the desing polylines
        if (m_input_mode == input_mode_t::mask) m_polyline.draw();

        // Draw the motion polyline
        m_found_motion.draw();
    }

    if (m_view == 2) {
        // Draw the detected rectangle
        ofDrawRectangle(m_max_rect.x, m_max_rect.y, m_max_rect.width, m_max_rect.height);
    }

    ofPopStyle();

    ofPushStyle();
    //  ofNoFill();
    ofSetLineWidth(1.0);
    // Draw the scaled mask in white
    if (m_view == 1 && m_show_mask_line) m_maskPolyLineScaled.draw();

    m_font.drawString(getStatusInfo(), 1, m_height + c_window_height_offset - 8);

    // Display the recording time
    if (m_recording) {
        ofSetColor(ofColor::white);
        m_font.drawString("REC: " + common::getElapsedTimeString(), m_width - 100,
                          m_height + c_window_height_offset - 8);

        if (m_timex_recording_point.elapsed()) {
            ofSetColor(ofColor::red);
            ofDrawCircle(m_width - 110, m_height + c_window_height_offset - 14, 6);

            m_timex_recording_point.set();
        }
    }

    // Draw the mask deisgn line
    if (m_input_mode == input_mode_t::mask && m_polyline.size()) {
        ofDrawLine(m_mask_linex, m_mask_liney, m_mouseX, m_mouseY);
    }

    ofPopStyle();
}

//--------------------------------------------------------------
string& ofApp::getStatusInfo()
{
    // clang-format off

    char buf[512];
    sprintf(buf,"%s %dx%d FPS/Frame: %2.2d/%2.2d / %d Q:%.3d [ %3d, %3d, %3d, %3d ] Video:%2d",
                m_cam.getCodeName().c_str(),
                (int)m_cam.getSize().width,
                (int)m_cam.getSize().height,
                (int)m_cam.getFPS(),
                (int) ofGetFrameRate(),
                m_framecount,
                0,//        (int)m_writer.get_queue().size(),
                m_config.settings.minthreshold,
                m_config.settings.minrectwidth,
                m_config.settings.mincontoursize,
                m_config.settings.detectionsmaxcount,
                m_recording_duration);

    // clang-format on

    m_statusinfo = buf;
    return m_statusinfo;
}

//--------------------------------------------------------------
void ofApp::saveDetectionImage()
{
    Rect r = toCv(m_detected.getBoundingBox());

    Mat img;
    m_frame.copyTo(img);
    common::bgr2rgb(img);

    string text = to_string(r.width) + "x" + to_string(r.height);

    cv::putText(img, text, cv::Point(r.x, r.y - 10), cv::FONT_HERSHEY_DUPLEX, 0.5,
                cv::Scalar(0, 255, 0), 0.5, false);

    string filename = m_cmd.get_filepath("motion_" + m_config.parameters.camname, ".jpg", 1);
    cv::rectangle(img, r, cv::Scalar(0, 0, 255), 2);

    imwrite(filename, img);
}

//--------------------------------------------------------------
void ofApp::on_mask_updated()
{
    const auto maskPoints = m_motion.getMaskPoints();
    m_maskPolyLineScaled.clear();

    if (maskPoints.size()) {
        for (const auto p : maskPoints) {
            m_maskPolyLineScaled.lineTo(p.x, p.y);
        }

        float scalex = static_cast<float>(m_width * 100 / m_motion.getWidth()) / 100;
        float scaley = static_cast<float>(m_height * 100 / m_motion.getHeight()) / 100;

        m_maskPolyLineScaled.scale(scalex, scaley);
    }
}

//--------------------------------------------------------------
void ofApp::on_motion(Rect& r)
{
    m_max_rect = r;
    m_found_motion = m_found_motion.fromRectangle(toOf(r));

    float sx = static_cast<float>(m_resized.cols * 100 / m_motion.getWidth()) / 100;
    float sy = static_cast<float>(m_resized.rows * 100 / m_motion.getHeight()) / 100;

    m_found_motion.scale(sx, sy);
}

//--------------------------------------------------------------
void ofApp::on_motion_detected(Rect& r)
{
    if (m_motion_detected) return;

    m_motion_detected = true;
    m_detected = m_detected.fromRectangle(toOf(r));

    float sx = static_cast<float>(m_frame.cols * 100 / m_motion.getWidth()) / 100;
    float sy = static_cast<float>(m_frame.rows * 100 / m_motion.getHeight()) / 100;

    m_detected.scale(sx, sy);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
    if (key == '1') {
        m_view = 1;
        return;
    }

    if (key == '2') {
        m_view = 2;
        return;
    }

    if (key == '3') {
        m_view = 3;
        return;
    }

    if (key == '4') {
        m_view = 4;
        return;
    }

    if (key == 'i') {
        m_show_mask_line = !m_show_mask_line;
        return;
    }

    if (OF_KEY_F5 == key) {
        m_maskPolyLineScaled.clear();
        m_motion.resetMask();
        m_input_mode = input_mode_t::mask;
        return;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y)
{
    if (m_input_mode != input_mode_t::mask) return;

    m_mouseX = x;
    m_mouseY = y;
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{
    if (m_input_mode == input_mode_t::mask) {
        if (button == 0) {
            //   if (x < 320 /*m_motion.getWidth()*/ && y < 240 /*m_motion.getHeight()*/) {
            m_polyline.lineTo(x, y);
            m_mask_linex = x;
            m_mask_liney = y;
            // }

        } else if (button == 2) {
            if (m_polyline.size() > 1) {
                // get the fisrt and last vertices
                auto v1 = m_polyline.getVertices()[0];
                auto v2 = m_polyline.getVertices().back();

                Point p1(v1.x, v1.y);
                Point p2(v2.x, v2.y);

                if (p1 != p2) m_polyline.lineTo(p1.x, p1.y);

                // scale down
                // clang-format off
                float scalex = static_cast<float>(m_motion.getWidth() * 100 / m_resized.cols) / 100;
                float scaley = static_cast<float>(m_motion.getHeight() * 100 / m_resized.rows) / 100;
                // clang-format on

                m_polyline.scale(scalex, scaley);

                vector<Point> maskPoints;
                for (const auto& v : m_polyline) {
                    maskPoints.push_back(Point(v.x, v.y));
                }

                m_config.mask_points = maskPoints;
                m_config.save();

                m_polyline.clear();
                m_motion.init();

                m_input_mode = input_mode_t::none;
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {}
