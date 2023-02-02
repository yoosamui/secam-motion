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

class Cmd : public ofThread
{
  public:
    void ffmpeg_start() { m_processing = true; }

    void ffmpeg_stop()
    {
        string cmd = "bash stopffmpeg.sh &";
        common::exec(cmd.c_str());
    }

    string get_filepath(const string& prefix, const string& extension, int ret = 0)
    {
        const string timestamp = common::getTimestamp(m_config.settings.timezone, "%T");
        const string timestampMillis = common::getTimestampMillis(m_config.settings.timezone, "%T");

        try {
            m_source_dir =
                m_config.settings.storage + common::getTimestamp(m_config.settings.timezone, "%F");
            boost::filesystem::create_directory(m_source_dir);

        } catch (boost::filesystem::filesystem_error& e) {
            common::log(e.what(), OF_LOG_ERROR);
        }

        m_file = "/" + timestampMillis + "_" + prefix + extension;

        return ret == 0 ? m_source_dir : m_source_dir + m_file;
    }

  private:
    Config& m_config = m_config.getInstance();
    string m_commandAsync;
    bool m_processing = false;
    string m_source_dir;
    string m_destination_dir;
    string m_file;

    void threadedFunction()
    {
        bool _commnand_set = false;

        while (isThreadRunning()) {
            while (m_processing) {
                if (!_commnand_set) {
                    _commnand_set = true;

                    string filename =
                        get_filepath("motion_" + m_config.parameters.camname, ".mp4", 1);

                    string cmd =
                        "bash startffmpeg.sh " + m_config.settings.uri + " '" + filename + "' &";

                    common::exec(cmd.c_str());
                    m_processing = false;
                }
            }
            ofSleepMillis(10);
        }
    }
};

