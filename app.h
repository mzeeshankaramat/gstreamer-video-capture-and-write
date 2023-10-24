#include <iostream>
#include <unistd.h>  // for STDIN_FILENO
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <sstream>
#include "opencv2/opencv.hpp"
#include <algorithm>
#include <cmath>

#include "opencv2/videoio.hpp"
#include "opencv2/video/background_segm.hpp"
#include <memory>

#include <opencv2/opencv.hpp>

class VideoHandler
{
public:
    std::shared_ptr<cv::VideoCapture> m_cap;
    std::shared_ptr<cv::VideoWriter> m_writer;

public:
    VideoHandler()
    {

    }

    ~VideoHandler()
    {
        // Release resources if necessary
        if (m_writer && m_writer->isOpened()) m_writer->release();
        if (m_cap && m_cap->isOpened()) m_cap->release();
    }

    void gstreamerDummyVideoCaptureAndShowInCV();
    void gstreamerRTSPVideoCaptureAndShowInCV(std::string url);
};