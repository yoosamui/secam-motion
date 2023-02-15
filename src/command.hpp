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
    }
};

//--------------------------------------------------------------
class CommandWriter : public ofThread
{
    queue<Mat> m_queue;

    bool m_processing = false;
    int m_count = 0;
    bool m_found = false;

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
        m_found = false;
        m_count = 0;
    }

    void add(const Mat& frame)
    {
        if (m_found || m_processing || frame.empty() || m_count > 100) {
            return;
        }

        Mat rgb;
        frame.copyTo(rgb);
        common::bgr2rgb(rgb);

        m_queue.push(rgb);
        m_processing = true;
    }

    void stop()
    {
        m_processing = false;
        string command = "bash stop-detector.sh " + m_directory;
        common::log(command);
        common::exec(command.c_str());
    }

    void threadedFunction()
    {
        while (isThreadRunning()) {
            while (m_processing) {
                while (m_queue.size()) {
                    Mat image = m_queue.front();
                    m_queue.pop();

                    char buff[512];
                    sprintf(buff, "%s/image%.2d.jpg", m_directory.c_str(), ++m_count);
                    string filename(buff);
                    imwrite(filename, image);
                    common::log("add probe:" + to_string(m_count));
                }

                string command = "bash start-detector.sh " + m_directory + " " + m_filename;
                common::log(command);
                auto result = common::exec(command.c_str());

                common::log("Detection finish. " + result);

                m_found = result == "1";
                m_processing = false;
            }
            ofSleepMillis(10);
        }
    }
};

