#pragma once

#include <queue>

#include "common.h"
#include "config.h"
#include "constants.h"
#include "ofMain.h"
#include "ofThread.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"

using namespace ofxCv;
using namespace cv;
using namespace std;

class ICommand
{
  public:
    virtual void start() = 0;
    virtual void stop() = 0;
};

class Command : public ofThread, ICommand
{
  protected:
    string m_source_dir;
    string m_destination_dir;
    string m_file;
    string m_command;

    Config& m_config = m_config.getInstance();

    bool m_processing = false;

  public:
    virtual void start() {}
    virtual void stop() {}

    void threadedFunction()
    {
        bool _commnand_set = false;

        while (isThreadRunning()) {
            while (m_processing) {
                if (!_commnand_set) {
                    _commnand_set = true;
                    common::exec(m_command.c_str());
                    common::log("CMD :" + m_command);
                }

                m_processing = false;
            }
            ofSleepMillis(100);
        }
    }
};

//--------------------------------------------------------------
class CommandRecording : public Command
{
  public:
    void start() override
    {
        if (m_processing) return;

        string filename = common::get_filepath("motion_" + m_config.parameters.camname, ".mp4", 1);
        m_command = "bash start-recorder.sh " + m_config.settings.uri + " '" + filename + "' ";

        m_processing = true;
    }

    void stop() override
    {
        m_command = "bash stop-recorder.sh";
        common::exec(m_command.c_str());
        common::log("Kill command executed ");
    }
};

//--------------------------------------------------------------
class CommandWriter : public ofThread
{
    queue<Mat> m_queue;

    bool m_processing = false;

    string m_filename;
    string m_directory;

    Config& m_config = m_config.getInstance();

  public:
    CommandWriter() {}

    void setPath()
    {
        m_filename = common::get_filepath("PERSON_" + m_config.parameters.camname, ".jpg", 1);
        m_directory = m_config.settings.storage +
                      common::getTimestamp(m_config.settings.timezone, "%F") + "/" +
                      m_config.parameters.camname + "-" +
                      common::getTimestamp(m_config.settings.timezone, "%T");

        boost::filesystem::create_directory(m_directory);
    }

    void add(const Mat& frame)
    {
        if (m_processing) return;

        if (m_queue.size() >= 12 || frame.empty()) {
            return;
        }

        Mat rgb;
        frame.copyTo(rgb);
        common::bgr2rgb(rgb);

        m_queue.push(rgb);
        common::log("add probe.");
    }

    void start()
    {
        if (m_processing) return;

        m_processing = true;
    }

    void threadedFunction()
    {
        while (isThreadRunning()) {
            while (m_processing) {
                int count = 0;
                if (!m_queue.empty()) {
                    common::log("Create images...");
                }
                while (m_queue.size()) {
                    Mat image = m_queue.front();
                    m_queue.pop();

                    string filename = m_directory + "/image" + to_string(++count) + ".jpg";
                    imwrite(filename, image);
                    cout << filename << endl;
                }

                string command = "bash start-detector.sh " + m_directory + " " + m_filename;
                common::exec(command.c_str());
                cout << "Detection finish." << endl;
                m_processing = false;
            }
            ofSleepMillis(10);
        }
    }
};

