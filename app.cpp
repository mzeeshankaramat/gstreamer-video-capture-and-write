#include "app.h"
#include <QVBoxLayout>

std::string url = "rtsp://localhost:8554/bars";
std::string fileName = "lena.bmp";

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("GStreamer Video Streamer");
    QVBoxLayout *layout = new QVBoxLayout(this);

    dummyVideoCaptureAndShowInCVButton = new QPushButton("Dummy Video Capture And Show In CV", this);
    connect(dummyVideoCaptureAndShowInCVButton, &QPushButton::clicked, this, &MainWindow::onDummyVideoCaptureAndShowInCVClicked);
    layout->addWidget(dummyVideoCaptureAndShowInCVButton);

    videoCaptureAndWriteWebcamButton = new QPushButton("Webcam Capture and Show In Qt", this);
    connect(videoCaptureAndWriteWebcamButton, &QPushButton::clicked, this, &MainWindow::onVideoCaptureAndWriteWebcam);
    layout->addWidget(videoCaptureAndWriteWebcamButton);

    videoCaptureFromRtspAndShowInCVButton = new QPushButton("Rtsp Video Capture and Show in CV", this);
    connect(videoCaptureFromRtspAndShowInCVButton, &QPushButton::clicked, this, &MainWindow::onVideoCaptureFromRtspAndShowInCV);
    layout->addWidget(videoCaptureFromRtspAndShowInCVButton);

    videoCaptureAndWriterFromRtspButton = new QPushButton("Rtsp Video Capture and Write to File", this);
    connect(videoCaptureAndWriterFromRtspButton, &QPushButton::clicked, this, &MainWindow::onVideoCaptureAndWriterFromRtsp);
    layout->addWidget(videoCaptureAndWriterFromRtspButton);

    videoCaptureAndWriterFromDummyRtspButton = new QPushButton("Dummy Rtsp Capture and Write to File", this);
    connect(videoCaptureAndWriterFromDummyRtspButton, &QPushButton::clicked, this, &MainWindow::onVideoCaptureAndWriterFromDummyRtsp);
    layout->addWidget(videoCaptureAndWriterFromDummyRtspButton);

    videoCaptureAndWriterFromDummyTestSrcButton = new QPushButton("Dummy Test Source Capture and Write to File", this);
    connect(videoCaptureAndWriterFromDummyTestSrcButton, &QPushButton::clicked, this, &MainWindow::onVideoCaptureAndWriterFromDummyTestSrc);
    layout->addWidget(videoCaptureAndWriterFromDummyTestSrcButton);

    mp4ViderWriterFromImageButton = new QPushButton("Mp4 Video Writer from Image", this);
    connect(mp4ViderWriterFromImageButton, &QPushButton::clicked, this, &MainWindow::onMp4ViderWriterFromImage);
    layout->addWidget(mp4ViderWriterFromImageButton);

    aviVideoWriterFromImageButton = new QPushButton("Avi Video Writer from Image", this);
    connect(aviVideoWriterFromImageButton, &QPushButton::clicked, this, &MainWindow::onAviVideoWriterFromImage);
    layout->addWidget(aviVideoWriterFromImageButton);

    setLayout(layout);
}

void MainWindow::onDummyVideoCaptureAndShowInCVClicked()
{
    vhandler.stop();
    vhandler.gstreamerDummyVideoCaptureAndShowInCV();
}

void MainWindow::onVideoCaptureAndWriteWebcam()
{
    vhandler.stop();
    vhandler.gstreamerVideoCaptureAndWriteWebcam();
}

void MainWindow::onVideoCaptureFromRtspAndShowInCV()
{
    vhandler.stop();
    vhandler.gstreamerRTSPVideoCaptureAndShowInQt(url);
}

void MainWindow::onVideoCaptureAndWriterFromRtsp()
{
    vhandler.stop();
    vhandler.gstreamerVideoCaptureAndWriterFromRtsp(url);
}

void MainWindow::onVideoCaptureAndWriterFromDummyRtsp()
{
    vhandler.stop();
    vhandler.gstreamerVideoCaptureAndWriterFromDummyRtsp();
}

void MainWindow::onVideoCaptureAndWriterFromDummyTestSrc()
{
    vhandler.stop();
    vhandler.gstreamerVideoCaptureAndWriterFromDummyTestSrc();
}

void MainWindow::onMp4ViderWriterFromImage()
{
    vhandler.stop();
    vhandler.gstreamerMp4ViderWriterFromImage(fileName);
}

void MainWindow::onAviVideoWriterFromImage()
{
    vhandler.stop();
    vhandler.gstreamerAviVideoCaptureAndWriterFromImage(fileName);
}

void GStreamerVideoHandle::stop()
{
    if (m_timer) {
        m_timer->stop();
    }

    if (m_cap && m_cap->isOpened()) {
        m_cap->release();
    }

    if (m_writer && m_writer->isOpened()) {
        m_writer->release();
    }
}

void GStreamerVideoHandle::gstreamerDummyVideoCaptureAndShowInCV()
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

void GStreamerVideoHandle::gstreamerRTSPVideoCaptureAndShowInCV(std::string url)
{
    std::string gst_str_capture = "rtspsrc location=" + url + " latency=200 "
    "! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! videoscale ! video/x-raw,width=1280,height=720 ! queue "
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

void GStreamerVideoHandle::gstreamerVideoCaptureAndWriteWebcam()
{
    m_label.show();

    std::string file_name = "output.mp4"; // Specify the output file name

    // use v4l2src argument device to use other than default camera
    std::string gst_str_capture = "v4l2src ! videoconvert ! video/x-raw,width=" + std::to_string(1280) + ",height=" + std::to_string(720) + " ! queue ! appsink drop=1";

    m_cap = std::make_shared<cv::VideoCapture>(gst_str_capture, cv::CAP_GSTREAMER);

    if (!m_cap->isOpened()) 
    {
        qDebug() << "cap not open";
        return;
    }

    cv::Mat first_frame;
    *m_cap >> first_frame;

    cv::Size input_size = first_frame.size();

    std::string gst_str_writer = "appsrc ! videoconvert ! videoscale ! video/x-raw,format=I420 ! x264enc "
        "! mp4mux ! filesink location=" + file_name;
    
    m_writer = std::make_shared<cv::VideoWriter>(gst_str_writer, cv::CAP_GSTREAMER, 0, 30, input_size, true);

    if (!m_writer->isOpened())
    {
        qDebug() << "writer not open";
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

        m_writer->write(frame);

        // Convert the captured frame to QImage
        QImage qImg = QImage((const unsigned char *)(frame.data), frame.cols, frame.rows, QImage::Format_RGB888).rgbSwapped();
        m_label.setPixmap(QPixmap::fromImage(qImg));
    });

    m_timer->start(10);  // Update every 30 ms
}

void GStreamerVideoHandle::gstreamerRTSPVideoCaptureAndShowInQt(std::string url)
{ 
    m_label.show();

    std::string gst_str_capture = "rtspsrc location=" + url + " latency=200 "
    "! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! videoscale ! video/x-raw,width=1280,height=720 ! queue "
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
        m_label.setPixmap(QPixmap::fromImage(qImg));
    });

    m_timer->start(10);  // Update every 30 ms
}



void GStreamerVideoHandle::gstreamerVideoCaptureAndWriterFromRtsp(std::string url)
{
    m_label.show();
    // std::string m_url = "rtsp://admin:admin@192.168.219.156:1935"; 
    std::string file_name = "output.mp4"; // Specify the output file name

    std::string gst_str_capture = "rtspsrc location=" + url + " latency=200 "
    "! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! videoscale ! video/x-raw,width=1280,height=720 ! queue "
    "! appsink drop=1";

    m_cap = std::make_shared<cv::VideoCapture>(gst_str_capture, cv::CAP_GSTREAMER);

    if (!m_cap->isOpened()) 
    {
        qDebug() << "cap not open";
        return;
    }

    cv::Mat first_frame;
    *m_cap >> first_frame;

    cv::Size input_size = first_frame.size();

    std::string gst_str_writer = "appsrc ! videoconvert ! videoscale ! video/x-raw,format=I420 ! x264enc "
        "! mp4mux ! filesink location=" + file_name;
    
    m_writer = std::make_shared<cv::VideoWriter>(gst_str_writer, cv::CAP_GSTREAMER, 0, 30, input_size, true);

    if (!m_writer->isOpened())
    {
        qDebug() << "writer not open";
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

        m_writer->write(frame);

        // Convert the captured frame to QImage
        QImage qImg = QImage((const unsigned char *)(frame.data), frame.cols, frame.rows, QImage::Format_RGB888).rgbSwapped();
        m_label.setPixmap(QPixmap::fromImage(qImg));
    });

    m_timer->start(10);  // Update every 30 ms
}

void GStreamerVideoHandle::gstreamerVideoCaptureAndWriterFromDummyRtsp()
{
    m_label.show();
    // dummy rtsp, service is running in background
    std::string url = "rtsp://localhost:8554/bars"; 

    // Specify the output file name
    std::string file_name = "output.mp4"; 

    std::string gst_str_capture = "rtspsrc location=" + url + " latency=200 "
    "! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! videoscale ! video/x-raw,width=1280,height=720 ! queue "
    "! appsink drop=1";

    m_cap = std::make_shared<cv::VideoCapture>(gst_str_capture, cv::CAP_GSTREAMER);

    if (!m_cap->isOpened()) 
    {
        qDebug() << "cap not open";
        return;
    }

    cv::Mat first_frame;
    *m_cap >> first_frame;

    cv::Size input_size = first_frame.size();

    std::string gst_str_writer = "appsrc ! videoconvert ! videoscale ! video/x-raw,format=I420 ! x264enc "
        "! mp4mux ! filesink location=" + file_name;
    
    m_writer = std::make_shared<cv::VideoWriter>(gst_str_writer, cv::CAP_GSTREAMER, 0, 30, input_size, true);

    if (!m_writer->isOpened())
    {
        qDebug() << "writer not open";
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

        m_writer->write(frame);

        // Convert the captured frame to QImage
        QImage qImg = QImage((const unsigned char *)(frame.data), frame.cols, frame.rows, QImage::Format_RGB888).rgbSwapped();
        m_label.setPixmap(QPixmap::fromImage(qImg));
    });

    m_timer->start(10);  // Update every 30 ms
}

void GStreamerVideoHandle::gstreamerVideoCaptureAndWriterFromDummyTestSrc()
{
    m_label.show();
    // Specify the output file name
    std::string file_name = "test.mp4"; 

    std::string gst_str_capture = "videotestsrc ! videoscale ! video/x-raw,width=1280,height=720 ! appsink drop=1";

    m_cap = std::make_shared<cv::VideoCapture>(gst_str_capture, cv::CAP_GSTREAMER);

    if (!m_cap->isOpened()) 
    {
        qDebug() << "cap not open";
        return;
    }

    cv::Mat first_frame;
    *m_cap >> first_frame;

    cv::Size input_size = first_frame.size();

    std::string gst_str_writer = "appsrc ! videoconvert ! x264enc ! mp4mux ! filesink location=" + file_name;
    
    m_writer = std::make_shared<cv::VideoWriter>(gst_str_writer, cv::CAP_GSTREAMER, 0, 30, input_size, true);

    if (!m_writer->isOpened())
    {
        qDebug() << "writer not open";
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

        m_writer->write(frame);

        // Convert the captured frame to QImage
        QImage qImg = QImage((const unsigned char *)(frame.data), frame.cols, frame.rows, QImage::Format_RGB888).rgbSwapped();
        m_label.setPixmap(QPixmap::fromImage(qImg));
    });

    m_timer->start(10); 
}

void GStreamerVideoHandle::gstreamerMp4ViderWriterFromImage(std::string imgName)
{
    m_label.show();
    std::string filePath = "resources/" + imgName;
    cv::Mat image = cv::imread(filePath);
    cv::Size input_size = image.size();

    std::string fileName = "output_image_video.mp4";

    std::string gst_str_writer = "appsrc ! videoconvert ! video/x-raw,format=I420 ! x264enc ! mp4mux ! filesink location=" + fileName;
    m_writer = std::make_shared<cv::VideoWriter>(gst_str_writer, cv::CAP_GSTREAMER, 0, 30, input_size, true);

    if (!m_writer->isOpened()) 
    {
        qDebug() << "writer not open";
        return;
    }

    if (image.empty())
    {
        qDebug() << "!!!Could not open or find the image" << endl;
        return;
    }

    int num_frames = 30;

    // Convert the captured frame to QImage
    QImage qImg = QImage((const unsigned char *)(image.data), image.cols, image.rows, QImage::Format_RGB888).rgbSwapped();
    m_label.setPixmap(QPixmap::fromImage(qImg));

    // Write the image to the video file multiple times
    for (int i = 0; i < num_frames; ++i) 
    {
        m_writer->write(image);
    }

    qDebug() << "File: "<< QString::fromStdString(fileName) << "has been written" << endl;
}

void GStreamerVideoHandle::gstreamerAviVideoCaptureAndWriterFromImage(std::string imgName)
{
    m_label.show();
    std::string filePath = "resources/" + imgName;
    cv::Mat image = cv::imread(filePath);
    cv::Size input_size = image.size();

    std::string fileName = "output_image_video.avi";

    std::string gst_str_writer = "appsrc ! videoconvert ! avimux ! filesink location=" + fileName;
    m_writer = std::make_shared<cv::VideoWriter>(gst_str_writer, cv::CAP_GSTREAMER, 0, 30, input_size, true);

    if (!m_writer->isOpened()) 
    {
        qDebug() << "writer not open";
        return;
    }

    if (image.empty())
    {
        qDebug() << "!!!Could not open or find the image" << endl;
        return;
    }

    int num_frames = 30;

    // Convert the captured frame to QImage
    QImage qImg = QImage((const unsigned char *)(image.data), image.cols, image.rows, QImage::Format_RGB888).rgbSwapped();
    m_label.setPixmap(QPixmap::fromImage(qImg));

    // Write the image to the video file multiple times
    for (int i = 0; i < num_frames; ++i) 
    {
        m_writer->write(image);
    }

    qDebug() << "File: "<< QString::fromStdString(fileName) << "has been written" << endl;
}
