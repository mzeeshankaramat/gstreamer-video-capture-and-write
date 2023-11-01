#pragma once

#include <QCoreApplication>
#include <QSocketNotifier>
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

#include <QApplication>
#include <QLabel>
#include <QImage>
#include <QTimer>
#include <QDebug>
#include <opencv2/opencv.hpp>

#include <QWidget>
#include <QPushButton>

class GStreamerVideoHandle
{
public:
    std::shared_ptr<cv::VideoCapture> m_cap;
    std::shared_ptr<cv::VideoWriter> m_writer;
    std::shared_ptr<QTimer> m_timer;
    QLabel m_label;

public:
    GStreamerVideoHandle()
    {
        m_label.setMinimumSize(1280, 720);
    }

    ~GStreamerVideoHandle()
    {
        // Release resources if necessary
        if (m_writer && m_writer->isOpened()) m_writer->release();
        if (m_cap && m_cap->isOpened()) m_cap->release();
    }

    void stop();
    void gstreamerDummyVideoCaptureAndShowInCV();
    void gstreamerRTSPVideoCaptureAndShowInCV(std::string url);
    void gstreamerRTSPVideoCaptureAndShowInQt(std::string url);
    void gstreamerVideoCaptureAndWriteWebcam();
    void gstreamerVideoCaptureAndWriterFromRtsp(std::string url);
    void gstreamerVideoCaptureAndWriterFromDummyRtsp();
    void gstreamerVideoCaptureAndWriterFromDummyTestSrc();
    void gstreamerMp4ViderWriterFromImage(std::string file_name);
    void gstreamerAviVideoCaptureAndWriterFromImage(std::string file_name);

};

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    
    ~MainWindow()
    {

    }

private slots:
    void onDummyVideoCaptureAndShowInCVClicked();
    void onVideoCaptureAndWriteWebcam();
    void onVideoCaptureFromRtspAndShowInCV();
    void onVideoCaptureAndWriterFromRtsp();
    void onVideoCaptureAndWriterFromDummyRtsp();
    void onVideoCaptureAndWriterFromDummyTestSrc();
    void onMp4ViderWriterFromImage();
    void onAviVideoWriterFromImage();


private:
    GStreamerVideoHandle vhandler;
    QPushButton *dummyVideoCaptureAndShowInCVButton;
    QPushButton *videoCaptureAndWriteWebcamButton;
    QPushButton *videoCaptureFromRtspAndShowInCVButton;
    QPushButton *videoCaptureAndWriterFromRtspButton;
    QPushButton *videoCaptureAndWriterFromDummyRtspButton;
    QPushButton *videoCaptureAndWriterFromDummyTestSrcButton;
    QPushButton *mp4ViderWriterFromImageButton;
    QPushButton *aviVideoWriterFromImageButton;
};
