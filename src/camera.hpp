#pragma once

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <string>

#include "ofxOpenCv.h"

using namespace std;
using namespace cv;

class Camera : public cv::VideoCapture
{
  public:
    float m_fps = 0.0;

    float getFPS() { return get(CAP_PROP_FPS); }

   
    

    Size getSize()
    {
        //
        return Size((int)get(CAP_PROP_FRAME_WIDTH), (int)get(CAP_PROP_FRAME_HEIGHT));
    }

    int getCodeType()
    {
        //
        return static_cast<int>(get(CAP_PROP_FOURCC));
    }

    string getCodeName()
    {
        int ex = getCodeType();
        char EXT[] = {(char)(ex & 0XFF), (char)((ex & 0XFF00) >> 8), (char)((ex & 0XFF0000) >> 16),
                      (char)((ex & 0XFF000000) >> 24), 0};

        return EXT;
    }

    
    string toString()
    {
        stringstream ss;

        Size s = getSize();
        string codec = getCodeName();
        ss << "codec " << codec << "size " << s.width << " x " << s.height << endl;

        return ss.str();
    }

  
};
