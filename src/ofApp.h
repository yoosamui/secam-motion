#pragma once

#include <iostream>
#include <string>

#include "camera.hpp"
#include "cmd.hpp"
#include "common.h"
#include "config.h"
#include "constants.h"
#include "motion.hpp"
#include "ofMain.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"

using namespace ofxCv;
using namespace std;
using namespace cv;

#include "ofMain.h"

class ofApp : public ofBaseApp
{
  private:
    enum input_mode_t { none, mask, motion };
    // thread* m_thread;
    int m_width = 640;
    int m_height = 360;
    int m_detections_count = 0;
    int m_recording_duration = c_videoduration;
    int m_mask_linex = 0;
    int m_mask_liney = 0;
    int m_mouseX = 0;
    int m_mouseY = 0;
    int m_view = 1;
    int m_framecount = 0;

    string m_statusinfo;

    bool m_recording = false;
    bool m_motion_detected = false;
    bool m_processing = false;
    bool m_show_mask_line = true;

    Rect m_max_rect;

    ofPolyline m_found_motion;
    ofPolyline m_detected;
    ofPolyline m_polyline;
    ofPolyline m_maskPolyLineScaled;

    Mat m_frame;
    Mat m_resized;

    Config& m_config = m_config.getInstance();
    ofTrueTypeFont m_font;
    input_mode_t m_input_mode = input_mode_t::none;

    // histoty = 500  VarThreshold = 16
    Ptr<cv::BackgroundSubtractorMOG2> mog2 = createBackgroundSubtractorMOG2();
    ContourFinder m_contour_finder;
    Camera m_cam;
    Cmd m_cmd;
    Motion m_motion;

    common::Timex m_timex_stoprecording;
    common::Timex m_timex_second;
    common::Timex m_timex_recording_point;

    string& getStatusInfo();
    void saveDetectionImage();

    void on_motion(Rect& r);
    void on_motion_detected(Rect& r);
    void on_mask_updated();

  public:
    void setup();
    void update();
    void draw();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    void ofExit();
};

