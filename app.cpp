#pragma once

#include "app.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QLabel label;
    label.show();

    std::string url = "rtsp://localhost:8554/bars";

    VideoHandler vhandler;
    // vhandler.gstreamerDummyVideoCaptureAndShowInCV();
    // vhandler.gstreamerRTSPVideoCaptureAndShowInCV(url);
    vhandler.gstreamerRTSPVideoCaptureAndShowInQt(url, label);


    return app.exec();
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

void VideoHandler::gstreamerRTSPVideoCaptureAndShowInQt(std::string url, QLabel &label)
{ 
    std::string gst_str_capture = "rtspsrc location=" + url + " latency=200 "
    "! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! queue "
    "! appsink drop=1";

    m_cap = std::make_shared<cv::VideoCapture>(gst_str_capture, cv::CAP_GSTREAMER);

    if (!m_cap->isOpened()) 
    {
        qDebug() << "cap not open";
        return;
    }

    m_timer = std::make_shared<QTimer>();
    QObject::connect(m_timer.get(), &QTimer::timeout, [&, this]()
    {
        cv::Mat frame;
        *m_cap >> frame;

        if (frame.empty())
        {
            return;
        }

        // Convert the captured frame to QImage
        QImage qImg = QImage((const unsigned char *)(frame.data), frame.cols, frame.rows, QImage::Format_RGB888).rgbSwapped();
        label.setPixmap(QPixmap::fromImage(qImg));
    });

    m_timer->start(10);  // Update every 30 ms
}
