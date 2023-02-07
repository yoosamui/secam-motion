#include <iomanip>
#include <iostream>
#include <opencv2/highgui.hpp>

//#include "constants.h"
#include "config.h"
#include "ofApp.h"
#include "ofAppNoWindow.h"
#include "ofMain.h"

namespace bfs = boost::filesystem;

static const string keys =
    "{ help help|       | print help message. }"
    "{ camera c |       | load the camera config.cfg file and starts streaming. }"
    "{ mode m   | 0     | start secam as client = 0 or server = 1 instance.}"
    "{ rec  r   | 0     | Use the internal video = 0, 1 = external recording.}"
    "{ dec  d   | 1     | Use the object detector.}"
    "{ fps  f   | 30    | frame rate.}"
    "{ width w  | 640   | view width. }"
    "{ height h | 360   | view height. }";

int main(int argc, char* argv[])
{
    cv::CommandLineParser parser(argc, argv, keys);
    parser.about("This is the secam client server system.");

    if (parser.has("help")) {
        parser.printMessage();
        return 0;
    }

    if (!parser.check()) {
        parser.printErrors();
        return 1;
    }

    auto camera = parser.get<std::string>("camera");
    auto width = parser.get<int>("width");
    auto height = parser.get<int>("height");
    auto mode = parser.get<int>("mode");
    auto fps = parser.get<int>("fps");
    auto recordmode = parser.get<int>("rec");

    if (fps < 5 || fps > 60) {
        cout << "invalid fps values." << endl;
        return 1;
    }

    if (height < 50 || width < 50) {
        cout << "invalid width, height values." << endl;
        return 1;
    }

    if (camera.empty() || camera == "true") {
        cout << "Enter the camera name. -c=mycamera" << endl;
        return 1;
    }

    if (!bfs::exists("data/" + camera + ".xml")) {
        std::cout << "Can't find " + camera + " config file." << std::endl;
        return 1;
    }

    auto logfile = "data/logs/" + camera + ".log";
    if (bfs::exists(logfile)) {
        bfs::remove(logfile);
    }

    auto logdir = "data/logs";
    if (!bfs::exists(logdir)) {
        bfs::create_directory(logdir);
    }

    ofAppNoWindow window;
    if (mode) {
        ofSetupOpenGL(&window, width, height, OF_WINDOW);
        cout << "start secam as server." << endl;
    } else {
        // add 340 Pixelfor the debug frames and for the status line
        ofSetupOpenGL(width + 1, height + c_window_height_offset, OF_WINDOW);
    }

    Config& m_config = m_config.getInstance();
    m_config.parameters.camname = camera;
    m_config.parameters.width = width;
    m_config.parameters.height = height;
    m_config.parameters.mode = mode;
    m_config.parameters.fps = fps;
    m_config.parameters.recordmode = recordmode;

    auto app = std::make_shared<ofApp>();
    ofRunApp(app);

    return 0;
}

