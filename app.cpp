#pragma once

#include "app.h"

int main(int argc, char *argv[])
{
    std::string url = "rtsp://localhost:8554/bars";

    VideoHandler vhandler;
    std::cout << cv::getBuildInformation() << std::endl;
    vhandler.gstreamerDummyVideoCaptureAndShowInCV();
    // vhandler.gstreamerRTSPVideoCaptureAndShowInCV(url);

    return 1;
}

void VideoHandler::gstreamerDummyVideoCaptureAndShowInCV()
{
    std::string gst_str_capture = "videotestsrc ! appsink drop=1";

    m_cap = std::make_shared<cv::VideoCapture>(gst_str_capture, cv::CAP_GSTREAMER);

    if (!m_cap->isOpened()) 
    {
        std::cout << "cap not open" << std::endl;;
        return;
    }

    cv::namedWindow("Live Stream", cv::WINDOW_AUTOSIZE); // Create a named window

    cv::Mat frame;

    while (true)
    {
        *m_cap >> frame;

        if (frame.empty())
        {
            return;
        }

        // Display the captured frame using OpenCV's imshow
        cv::imshow("Live Stream", frame);
        
        // Press 'ESC' key to exit
        char c = (char) cv::waitKey(10);
        if (c == 27)
        {
            break;
        }
    }

    cv::destroyAllWindows();
}

void VideoHandler::gstreamerRTSPVideoCaptureAndShowInCV(std::string url)
{
    std::string gst_str_capture = "rtspsrc location=" + url + " latency=200 "
    "! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! queue "
    "! appsink drop=1";

    m_cap = std::make_shared<cv::VideoCapture>(gst_str_capture, cv::CAP_GSTREAMER);

    if (!m_cap->isOpened()) 
    {
        std::cout << "cap not open" << std::endl;
        return;
    }

    cv::namedWindow("Live Stream", cv::WINDOW_AUTOSIZE); // Create a named window

    cv::Mat frame;

    while (true)
    {
        *m_cap >> frame;

        if (frame.empty())
        {
            return;
        }

        // Display the captured frame using OpenCV's imshow
        cv::imshow("Live Stream", frame);
        
        // Press 'ESC' key to exit
        char c = (char) cv::waitKey(10);
        if (c == 27)
        {
            break;
        }
    }

    cv::destroyAllWindows();
}
